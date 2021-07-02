#include "websocketpp/common/connection_hdl.hpp"
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/control.hpp>
#include <sbash64/budget/format.hpp>
#include <sbash64/budget/parse.hpp>
#include <sbash64/budget/serialization.hpp>

#include <nlohmann/json.hpp>

#define ASIO_STANDALONE
#include <utility>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/logger/syslog.hpp>
#include <websocketpp/server.hpp>

#include <fstream>
#include <iostream>
#include <map>
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

namespace sbash64::budget {
namespace {
class FileStreamFactory : public IoStreamFactory {
public:
  auto makeInput() -> std::shared_ptr<std::istream> override {
    return std::make_shared<std::ifstream>("/home/seth/budget.txt");
  }

  auto makeOutput() -> std::shared_ptr<std::ostream> override {
    return std::make_shared<std::ofstream>("/home/seth/budget.txt");
  }
};

auto put(std::ostream &stream, std::string_view name, std::string_view method,
         const Transaction &t) -> std::ostream & {
  nlohmann::json json;
  json["description"] = t.description;
  std::stringstream amountStream;
  amountStream << t.amount;
  json["amount"] = amountStream.str();
  std::stringstream dateStream;
  dateStream << t.date;
  json["date"] = dateStream.str();
  return stream << name << ' ' << method << ' ' << json;
}

class WebSocketAccountObserver : public Account::Observer {
public:
  WebSocketAccountObserver(websocketpp::server<debug_custom> &server,
                           websocketpp::connection_hdl connection,
                           Account &account, std::string_view name)
      : connection{std::move(connection)}, server{server}, name{name} {
    account.attach(this);
  }

  void notifyThatBalanceHasChanged(USD usd) override {
    std::stringstream stream;
    stream << name << " updateBalance " << usd;
    server.send(connection, stream.str(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatCreditHasBeenAdded(const Transaction &t) override {
    std::stringstream stream;
    put(stream, name, "addCredit", t);
    server.send(connection, stream.str(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatDebitHasBeenAdded(const Transaction &t) override {
    std::stringstream stream;
    put(stream, name, "addDebit", t);
    server.send(connection, stream.str(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatDebitHasBeenRemoved(const Transaction &t) override {
    std::stringstream stream;
    put(stream, name, "removeDebit", t);
    server.send(connection, stream.str(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatCreditHasBeenRemoved(const Transaction &t) override {
    std::stringstream stream;
    put(stream, name, "removeCredit", t);
    server.send(connection, stream.str(),
                websocketpp::frame::opcode::value::text);
  }

private:
  websocketpp::connection_hdl connection;
  websocketpp::server<debug_custom> &server;
  std::string name;
};

class WebSocketModelObserver : public Model::Observer {
public:
  WebSocketModelObserver(websocketpp::server<debug_custom> &server,
                         websocketpp::connection_hdl connection, Model &model)
      : connection{std::move(connection)}, server{server} {
    model.attach(this);
  }

  void notifyThatNewAccountHasBeenCreated(Account &account,
                                          std::string_view name) override {
    accountObservers[std::string{name}] =
        std::make_unique<WebSocketAccountObserver>(server, connection, account,
                                                   name);
    std::stringstream stream;
    stream << "addAccount " << name;
    server.send(connection, stream.str(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatTotalBalanceHasChanged(USD usd) override {
    std::stringstream stream;
    stream << "updateBalance " << usd;
    server.send(connection, stream.str(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatAccountHasBeenRemoved(std::string_view name) override {
    std::stringstream stream;
    stream << "removeAccount " << name;
    server.send(connection, stream.str(),
                websocketpp::frame::opcode::value::text);
    accountObservers.at(std::string{name}).reset();
  }

private:
  websocketpp::connection_hdl connection;
  websocketpp::server<debug_custom> &server;
  std::map<std::string, std::unique_ptr<WebSocketAccountObserver>, std::less<>>
      accountObservers;
};

struct App {
  InMemoryAccount::Factory accountFactory;
  Bank bank{accountFactory};
  FileStreamFactory streamFactory;
  WritesAccountToStream::Factory accountSerializationFactory;
  WritesSessionToStream sessionSerialization{streamFactory,
                                             accountSerializationFactory};
  ReadsAccountFromStream::Factory accountDeserializationFactory;
  ReadsSessionFromStream accountDeserialization{streamFactory,
                                                accountDeserializationFactory};
  WebSocketModelObserver webSocketNotifier;

  App(websocketpp::server<debug_custom> &server,
      websocketpp::connection_hdl connection)
      : webSocketNotifier{server, std::move(connection), bank} {
    bank.load(accountDeserialization);
  }
};
} // namespace
} // namespace sbash64::budget

int main() {
  std::map<void *, std::unique_ptr<sbash64::budget::App>> applications;
  // Create a server endpoint
  websocketpp::server<debug_custom> server;

  try {
    // Set logging settings
    server.set_access_channels(websocketpp::log::alevel::all);
    server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    // Initialize ASIO
    server.init_asio();
    server.set_reuse_addr(true);

    server.set_open_handler([&server, &applications](
                                websocketpp::connection_hdl connection) {
      applications[connection.lock().get()] =
          std::make_unique<sbash64::budget::App>(server, std::move(connection));
    });

    server.set_fail_handler([&server](websocketpp::connection_hdl connection) {
      websocketpp::server<debug_custom>::connection_ptr con =
          server.get_con_from_hdl(std::move(connection));

      std::cout << "Fail handler: " << con->get_ec() << " "
                << con->get_ec().message() << std::endl;
    });

    server.set_close_handler(
        [&applications](websocketpp::connection_hdl connection) {
          std::cout << "Close handler" << std::endl;
          applications[connection.lock().get()].reset();
        });

    // Register our message handler
    server.set_message_handler(
        [&server](websocketpp::connection_hdl connection,
                  websocketpp::server<debug_custom>::message_ptr message) {
          std::cout << "on_message called with hdl: " << connection.lock().get()
                    << " and message: " << message->get_payload() << std::endl;

          try {
            server.send(connection, message->get_payload(),
                        message->get_opcode());
          } catch (websocketpp::exception const &e) {
            std::cout << "Echo failed because: "
                      << "(" << e.what() << ")" << std::endl;
          }
        });

    server.set_http_handler([&server](websocketpp::connection_hdl connection) {
      websocketpp::server<debug_custom>::connection_ptr con =
          server.get_con_from_hdl(std::move(connection));
      std::string res = con->get_request_body();

      std::stringstream ss;
      ss << "got HTTP request with " << res.size() << " bytes of body data.";

      con->set_body(ss.str());
      con->set_status(websocketpp::http::status_code::ok);
      std::cout << "get_host: " << con->get_host() << '\n';
      // std::cout << "get_origin: " << con->get_origin() << '\n';
      std::cout << "get_resource: " << con->get_resource() << '\n';
      std::cout << "get_subprotocol: " << con->get_subprotocol() << '\n';
      std::cout << "get_uri: " << con->get_uri()->str() << '\n';
      std::cout << "get_remote_endpoint: " << con->get_remote_endpoint()
                << '\n';
    });

    server.listen(9012);
    server.start_accept();
    server.run();
  } catch (websocketpp::exception const &e) {
    std::cout << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cout << "other exception" << std::endl;
  }
}
