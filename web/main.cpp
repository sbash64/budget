#include "websocketpp/common/connection_hdl.hpp"
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/control.hpp>
#include <sbash64/budget/format.hpp>
#include <sbash64/budget/parse.hpp>
#include <sbash64/budget/serialization.hpp>

#define ASIO_STANDALONE
#include <utility>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/logger/syslog.hpp>
#include <websocketpp/server.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <utility>

struct debug_custom : public websocketpp::config::debug_asio {
  using type = debug_custom;
  using base = debug_asio;

  using concurrency_type = base::concurrency_type;

  using request_type = base::request_type;
  using response_type = base::response_type;

  using message_type = base::message_type;
  using con_msg_manager_type = base::con_msg_manager_type;
  using endpoint_msg_manager_type = base::endpoint_msg_manager_type;

  using alog_type = base::alog_type;
  using elog_type = base::elog_type;

  using rng_type = base::rng_type;

  struct transport_config : public base::transport_config {
    using concurrency_type = type::concurrency_type;
    using alog_type = type::alog_type;
    using elog_type = type::elog_type;
    using request_type = type::request_type;
    using response_type = type::response_type;
    using socket_type = websocketpp::transport::asio::basic_socket::endpoint;
  };

  using transport_type =
      websocketpp::transport::asio::endpoint<transport_config>;

  static const long timeout_open_handshake = 0;
};

using server = websocketpp::server<debug_custom>;
using message_ptr = server::message_ptr;

namespace {
class FileStreamFactory : public sbash64::budget::IoStreamFactory {
public:
  auto makeInput() -> std::shared_ptr<std::istream> override {
    return std::make_shared<std::ifstream>("/home/seth/budget.txt");
  }

  auto makeOutput() -> std::shared_ptr<std::ostream> override {
    return std::make_shared<std::ofstream>("/home/seth/budget.txt");
  }
};

class WebSocketNotifier : public sbash64::budget::Bank::Observer {
public:
  explicit WebSocketNotifier(server *s) : s{s} {}

  void notifyThatNewAccountHasBeenCreated(sbash64::budget::Account &,
                                          std::string_view name) override {
    std::stringstream stream;
    stream << "account added: " << name;
    s->send(connection_, stream.str(), websocketpp::frame::opcode::value::text);
  }

  void notifyThatTotalBalanceHasChanged(sbash64::budget::USD usd) override {
    std::stringstream stream;
    stream << "balance is now " << usd;
    s->send(connection_, stream.str(), websocketpp::frame::opcode::value::text);
  }

  void notifyThatAccountHasBeenRemoved(std::string_view name) override {}

  void setConnection(websocketpp::connection_hdl connection) {
    connection_ = std::move(connection);
  }

private:
  websocketpp::connection_hdl connection_;
  server *s;
};
} // namespace

int main() {
  sbash64::budget::InMemoryAccount::Factory accountFactory;
  sbash64::budget::Bank bank{accountFactory};
  FileStreamFactory streamFactory;
  sbash64::budget::WritesAccountToStream::Factory accountSerializationFactory;
  sbash64::budget::WritesSessionToStream sessionSerialization{
      streamFactory, accountSerializationFactory};
  sbash64::budget::ReadsAccountFromStream::Factory
      accountDeserializationFactory;
  sbash64::budget::ReadsSessionFromStream accountDeserialization{
      streamFactory, accountDeserializationFactory};

  // Create a server endpoint
  server echo_server;
  WebSocketNotifier webSocketNotifier{&echo_server};
  bank.attach(&webSocketNotifier);

  try {
    // Set logging settings
    echo_server.set_access_channels(websocketpp::log::alevel::all);
    echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    // Initialize ASIO
    echo_server.init_asio();
    echo_server.set_reuse_addr(true);

    echo_server.set_open_handler(
        [&bank, &accountDeserialization,
         &webSocketNotifier](websocketpp::connection_hdl connection) {
          std::cout << "Open handler" << std::endl;
          webSocketNotifier.setConnection(std::move(connection));
          bank.load(accountDeserialization);
        });

    echo_server.set_fail_handler(
        [&echo_server](websocketpp::connection_hdl connection) {
          server::connection_ptr con = echo_server.get_con_from_hdl(
              std::forward<decltype(connection)>(connection));

          std::cout << "Fail handler: " << con->get_ec() << " "
                    << con->get_ec().message() << std::endl;
        });

    echo_server.set_close_handler([](websocketpp::connection_hdl) {
      std::cout << "Close handler" << std::endl;
    });

    // Register our message handler
    echo_server.set_message_handler(
        [&echo_server](websocketpp::connection_hdl connection,
                       message_ptr message) {
          std::cout << "on_message called with hdl: " << connection.lock().get()
                    << " and message: " << message->get_payload() << std::endl;

          try {
            echo_server.send(connection, message->get_payload(),
                             message->get_opcode());
          } catch (websocketpp::exception const &e) {
            std::cout << "Echo failed because: "
                      << "(" << e.what() << ")" << std::endl;
          }
        });

    echo_server.set_http_handler([&echo_server](
                                     websocketpp::connection_hdl connection) {
      server::connection_ptr con =
          echo_server.get_con_from_hdl(std::move(connection));

      std::string res = con->get_request_body();

      std::stringstream ss;
      ss << "got HTTP request with " << res.size() << " bytes of body data.";

      con->set_body(ss.str());
      con->set_status(websocketpp::http::status_code::ok);
    });

    // Listen on port 9012
    echo_server.listen(9012);

    // Start the server accept loop
    echo_server.start_accept();

    // Start the ASIO io_service run loop
    echo_server.run();
  } catch (websocketpp::exception const &e) {
    std::cout << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cout << "other exception" << std::endl;
  }
}
