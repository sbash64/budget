#define ASIO_STANDALONE
#include <iostream>
#include <utility>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/logger/syslog.hpp>
#include <websocketpp/server.hpp>

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

static bool validate(server *, websocketpp::connection_hdl) { return true; }

static void on_http(server *s, websocketpp::connection_hdl hdl) {
  server::connection_ptr con = s->get_con_from_hdl(std::move(hdl));

  std::string res = con->get_request_body();

  std::stringstream ss;
  ss << "got HTTP request with " << res.size() << " bytes of body data.";

  con->set_body(ss.str());
  con->set_status(websocketpp::http::status_code::ok);
}

static void on_fail(server *s, websocketpp::connection_hdl hdl) {
  server::connection_ptr con = s->get_con_from_hdl(std::move(hdl));

  std::cout << "Fail handler: " << con->get_ec() << " "
            << con->get_ec().message() << std::endl;
}

static void on_close(websocketpp::connection_hdl) {
  std::cout << "Close handler" << std::endl;
}

// Define a callback to handle incoming messages
static void on_message(server *s, const websocketpp::connection_hdl &hdl,
                       const message_ptr &msg) {
  std::cout << "on_message called with hdl: " << hdl.lock().get()
            << " and message: " << msg->get_payload() << std::endl;

  try {
    s->send(hdl, msg->get_payload(), msg->get_opcode());
  } catch (websocketpp::exception const &e) {
    std::cout << "Echo failed because: "
              << "(" << e.what() << ")" << std::endl;
  }
}

int main() {
  // Create a server endpoint
  server echo_server;

  try {
    // Set logging settings
    echo_server.set_access_channels(websocketpp::log::alevel::all);
    echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    // Initialize ASIO
    echo_server.init_asio();
    echo_server.set_reuse_addr(true);

    // Register our message handler
    echo_server.set_message_handler(
        [capture0 = &echo_server](auto &&PH1, auto &&PH2) {
          return on_message(capture0, std::forward<decltype(PH1)>(PH1),
                            std::forward<decltype(PH2)>(PH2));
        });

    echo_server.set_http_handler([capture0 = &echo_server](auto &&PH1) {
      return on_http(capture0, std::forward<decltype(PH1)>(PH1));
    });
    echo_server.set_fail_handler([capture0 = &echo_server](auto &&PH1) {
      return on_fail(capture0, std::forward<decltype(PH1)>(PH1));
    });
    echo_server.set_close_handler(&on_close);

    echo_server.set_validate_handler([capture0 = &echo_server](auto &&PH1) {
      return validate(capture0, std::forward<decltype(PH1)>(PH1));
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
