#include <sbash64/budget/account.hpp>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/control.hpp>
#include <sbash64/budget/format.hpp>
#include <sbash64/budget/parse.hpp>
#include <sbash64/budget/serialization.hpp>

#include <nlohmann/json.hpp>

#define ASIO_STANDALONE
#include <utility>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/logger/syslog.hpp>
#include <websocketpp/server.hpp>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
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

class Child {
public:
  virtual auto index() -> long = 0;
};

class Parent {
public:
  virtual auto index(void *) -> long = 0;
  virtual void release(void *) = 0;
};

class ParentAndChild : public Parent, public Child {
public:
  using Child::index;
  using Parent::index;
};

class WebSocketTransactionRecordObserver : public TransactionRecord::Observer {
public:
  WebSocketTransactionRecordObserver(websocketpp::server<debug_custom> &server,
                                     websocketpp::connection_hdl connection,
                                     TransactionRecord &record,
                                     ParentAndChild &parent)
      : connection{std::move(connection)}, server{server}, parent{parent} {
    record.attach(this);
  }

  void notifyThatIsVerified() override {
    nlohmann::json json;
    json["method"] = "verify transaction";
    json["accountIndex"] = parent.index();
    json["transactionIndex"] = parent.index(this);
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatIs(const Transaction &t) override {
    nlohmann::json json;
    json["description"] = t.description;
    std::stringstream amountStream;
    amountStream << t.amount;
    json["amount"] = amountStream.str();
    std::stringstream dateStream;
    dateStream << t.date;
    json["date"] = dateStream.str();
    json["method"] = "update transaction";
    json["accountIndex"] = parent.index();
    json["transactionIndex"] = parent.index(this);
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatWillBeRemoved() override {
    nlohmann::json json;
    json["method"] = "remove transaction";
    json["accountIndex"] = parent.index();
    json["transactionIndex"] = parent.index(this);
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
    parent.release(this);
  }

private:
  websocketpp::connection_hdl connection;
  websocketpp::server<debug_custom> &server;
  ParentAndChild &parent;
};

class WebSocketAccountObserver : public Account::Observer,
                                 public ParentAndChild {
public:
  WebSocketAccountObserver(websocketpp::server<debug_custom> &server,
                           websocketpp::connection_hdl connection,
                           Account &account, Parent &parent)
      : connection{std::move(connection)}, server{server}, parent{parent} {
    account.attach(this);
  }

  auto index(void *a) -> long override {
    return std::distance(
        children.begin(),
        std::find_if(
            children.begin(), children.end(),
            [a](const std::shared_ptr<WebSocketTransactionRecordObserver>
                    &child) { return child.get() == a; }));
  }

  auto index() -> long override { return parent.index(this); }

  void release(void *a) override {
    children.erase(std::find_if(
        children.begin(), children.end(),
        [&](const std::shared_ptr<WebSocketTransactionRecordObserver> &child)
            -> bool { return child.get() == a; }));
  }

  void notifyThatBalanceHasChanged(USD usd) override {
    nlohmann::json json;
    json["method"] = "update account balance";
    json["accountIndex"] = parent.index(this);
    std::stringstream amountStream;
    amountStream << usd;
    json["amount"] = amountStream.str();
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatCreditHasBeenAdded(TransactionRecord &t) override {
    children.push_back(std::make_shared<WebSocketTransactionRecordObserver>(
        server, connection, t, *this));
    nlohmann::json json;
    json["method"] = "add credit";
    json["accountIndex"] = parent.index(this);
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatDebitHasBeenAdded(TransactionRecord &t) override {
    children.push_back(std::make_shared<WebSocketTransactionRecordObserver>(
        server, connection, t, *this));
    nlohmann::json json;
    json["method"] = "add debit";
    json["accountIndex"] = parent.index(this);
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatWillBeRemoved() override {
    nlohmann::json json;
    json["method"] = "remove account";
    json["accountIndex"] = parent.index(this);
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
    parent.release(this);
  }

private:
  websocketpp::connection_hdl connection;
  websocketpp::server<debug_custom> &server;
  std::vector<std::shared_ptr<WebSocketTransactionRecordObserver>> children;
  Parent &parent;
};

class WebSocketModelObserver : public Budget::Observer, public Parent {
public:
  WebSocketModelObserver(websocketpp::server<debug_custom> &server,
                         websocketpp::connection_hdl connection, Budget &Budget)
      : connection{std::move(connection)}, server{server} {
    Budget.attach(this);
  }

  auto index(void *a) -> long override {
    return std::distance(
        children.begin(),
        std::find_if(
            children.begin(), children.end(),
            [a](const std::shared_ptr<WebSocketAccountObserver> &child) {
              return child.get() == a;
            }));
  }

  void release(void *a) override {
    children.erase(std::find_if(
        children.begin(), children.end(),
        [&](const std::shared_ptr<WebSocketAccountObserver> &child) -> bool {
          return child.get() == a;
        }));
  }

  void notifyThatNewAccountHasBeenCreated(Account &account,
                                          std::string_view name) override {
    children.push_back(std::make_shared<WebSocketAccountObserver>(
        server, connection, account, *this));
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
    json["method"] = "update total balance";
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

private:
  websocketpp::connection_hdl connection;
  websocketpp::server<debug_custom> &server;
  std::vector<std::shared_ptr<WebSocketAccountObserver>> children;
};

struct App {
  TransactionRecordInMemory::Factory transactionFactory;
  InMemoryAccount::Factory accountFactory;
  BudgetInMemory bank{accountFactory, transactionFactory};
  FileStreamFactory streamFactory;
  WritesTransactionRecordToStream::Factory
      transactionRecordSerializationFactory;
  WritesAccountToStream::Factory accountSerializationFactory{
      transactionRecordSerializationFactory};
  WritesSessionToStream sessionSerialization{streamFactory,
                                             accountSerializationFactory};
  ReadsTransactionRecordFromStream::Factory
      transactionRecordDeserializationFactory;
  ReadsAccountFromStream::Factory accountDeserializationFactory{
      transactionRecordDeserializationFactory};
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
  int year = 0;
  int month = 0;
  int day = 0;
  if (s.find('-') != std::string_view::npos) {
    stream >> year;
    stream.get();
    stream >> month;
    stream.get();
    stream >> day;
  } else {
    stream >> month;
    stream.get();
    stream >> day;
    stream.get();
    stream >> year;
  }
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
    const std::function<void(sbash64::budget::Budget &)> &f) {
  f(applications.at(connection.lock().get())->bank);
}

static auto methodIs(const nlohmann::json &json, std::string_view method)
    -> bool {
  return json["method"].get<std::string>() == method;
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
          if (methodIs(json, "debit"))
            call(applications, connection,
                 [&json](sbash64::budget::Budget &Budget) {
                   Budget.debit(json["name"].get<std::string>(),
                                sbash64::budget::transaction(json));
                 });
          else if (methodIs(json, "remove debit"))
            call(applications, connection,
                 [&json](sbash64::budget::Budget &Budget) {
                   Budget.removeDebit(json["name"].get<std::string>(),
                                      sbash64::budget::transaction(json));
                 });
          else if (methodIs(json, "credit"))
            call(applications, connection,
                 [&json](sbash64::budget::Budget &Budget) {
                   Budget.credit(sbash64::budget::transaction(json));
                 });
          else if (methodIs(json, "remove credit"))
            call(applications, connection,
                 [&json](sbash64::budget::Budget &Budget) {
                   Budget.removeCredit(sbash64::budget::transaction(json));
                 });
          else if (methodIs(json, "transfer"))
            call(applications, connection,
                 [&json](sbash64::budget::Budget &Budget) {
                   Budget.transferTo(
                       json["name"].get<std::string>(),
                       sbash64::budget::usd(json["amount"].get<std::string>()),
                       sbash64::budget::date(json["date"].get<std::string>()));
                 });
          else if (methodIs(json, "remove transfer"))
            call(applications, connection,
                 [&json](sbash64::budget::Budget &Budget) {
                   Budget.removeTransfer(
                       json["name"].get<std::string>(),
                       sbash64::budget::usd(json["amount"].get<std::string>()),
                       sbash64::budget::date(json["date"].get<std::string>()));
                 });
          else if (methodIs(json, "remove account"))
            call(applications, connection,
                 [&json](sbash64::budget::Budget &Budget) {
                   Budget.removeAccount(json["name"].get<std::string>());
                 });
          else if (methodIs(json, "create account"))
            call(applications, connection,
                 [&json](sbash64::budget::Budget &Budget) {
                   Budget.transferTo(
                       json["name"].get<std::string>(),
                       sbash64::budget::usd(json["amount"].get<std::string>()),
                       sbash64::budget::date(json["date"].get<std::string>()));
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
