#include "websocketpp/common/connection_hdl.hpp"
#include <functional>
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

auto json(std::string_view name, std::string_view method, const Transaction &t)
    -> nlohmann::json {
  nlohmann::json json;
  json["description"] = t.description;
  std::stringstream amountStream;
  amountStream << t.amount;
  json["amount"] = amountStream.str();
  std::stringstream dateStream;
  dateStream << t.date;
  json["date"] = dateStream.str();
  json["name"] = name;
  json["method"] = method;
  return json;
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
    nlohmann::json json;
    json["name"] = name;
    std::stringstream amountStream;
    amountStream << usd;
    json["amount"] = amountStream.str();
    json["method"] = "update account balance";
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatCreditHasBeenAdded(const Transaction &t) override {
    server.send(connection, json(name, "add credit", t).dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatDebitHasBeenAdded(const Transaction &t) override {
    server.send(connection, json(name, "add debit", t).dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatDebitHasBeenRemoved(const Transaction &t) override {
    server.send(connection, json(name, "remove debit", t).dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatCreditHasBeenRemoved(const Transaction &t) override {
    server.send(connection, json(name, "remove credit", t).dump(),
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
    nlohmann::json json;
    json["name"] = name;
    json["method"] = "add account";
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatTotalBalanceHasChanged(USD usd) override {
    nlohmann::json json;
    std::stringstream amountStream;
    amountStream << usd;
    json["amount"] = amountStream.str();
    json["method"] = "update balance";
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatAccountHasBeenRemoved(std::string_view name) override {
    nlohmann::json json;
    json["name"] = name;
    json["method"] = "remove account";
    server.send(connection, json.dump(),
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

static auto date(std::string_view s) -> Date {
  Date date{};
  std::stringstream stream{std::string{s}};
  int month = 0;
  stream >> month;
  stream.get();
  int day = 0;
  stream >> day;
  stream.get();
  int year = 0;
  stream >> year;
  date.month = Month{month};
  date.day = day;
  date.year = year;
  return date;
}

static auto transaction(const nlohmann::json &json) -> Transaction {
  return {usd(json["amount"].get<std::string>()),
          json["description"].get<std::string>(),
          date(json["date"].get<std::string>())};
}
} // namespace sbash64::budget

static void call(
    const std::map<void *, std::unique_ptr<sbash64::budget::App>> &applications,
    const websocketpp::connection_hdl &connection,
    const std::function<void(sbash64::budget::Bank &)> &f) {
  f(applications.at(connection.lock().get())->bank);
}

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

    server.set_open_handler(
        [&server, &applications](websocketpp::connection_hdl connection) {
          applications[connection.lock().get()] =
              std::make_unique<sbash64::budget::App>(server, connection);
        });

    server.set_fail_handler([&server](websocketpp::connection_hdl connection) {
      websocketpp::server<debug_custom>::connection_ptr con =
          server.get_con_from_hdl(std::move(connection));

      std::cout << "Fail handler: " << con->get_ec() << " "
                << con->get_ec().message() << std::endl;
    });

    server.set_close_handler(
        [&applications](websocketpp::connection_hdl connection) {
          applications.at(connection.lock().get()).reset();
        });

    // Register our message handler
    server.set_message_handler(
        [&applications](
            websocketpp::connection_hdl connection,
            websocketpp::server<debug_custom>::message_ptr message) {
          const auto json{nlohmann::json::parse(message->get_payload())};
          if (json["method"].get<std::string>() == "debit")
            call(applications, connection,
                 [&json](sbash64::budget::Model &model) {
                   model.debit(json["name"].get<std::string>(),
                               sbash64::budget::transaction(json));
                 });
          else if (json["method"].get<std::string>() == "credit")
            call(applications, connection,
                 [&json](sbash64::budget::Model &model) {
                   model.credit(sbash64::budget::transaction(json));
                 });
          if (json["method"].get<std::string>() == "remove debit")
            call(applications, connection,
                 [&json](sbash64::budget::Model &model) {
                   model.removeDebit(json["name"].get<std::string>(),
                                     sbash64::budget::transaction(json));
                 });
          else if (json["method"].get<std::string>() == "remove credit")
            call(applications, connection,
                 [&json](sbash64::budget::Model &model) {
                   model.removeCredit(sbash64::budget::transaction(json));
                 });
        });

    server.set_http_handler([&server](websocketpp::connection_hdl connection) {
      websocketpp::server<debug_custom>::connection_ptr con =
          server.get_con_from_hdl(std::move(connection));

      if (con->get_resource() == "/") {
        std::ifstream response{"index.html"};
        std::ostringstream stream;
        stream << response.rdbuf();
        con->set_body(stream.str());
      }
      if (con->get_resource() == "/main.js") {
        std::ifstream response{"main.js"};
        std::ostringstream stream;
        stream << response.rdbuf();
        con->set_body(stream.str());
      }
      con->set_status(websocketpp::http::status_code::ok);
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
