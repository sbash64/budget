#include <sbash64/budget/account.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/format.hpp>
#include <sbash64/budget/parse.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sbash64/budget/transaction.hpp>

#include <nlohmann/json.hpp>

#define ASIO_STANDALONE
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/logger/syslog.hpp>
#include <websocketpp/server.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
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
  explicit FileStreamFactory(std::string filePath)
      : filePath{std::move(filePath)} {}

  auto makeInput() -> std::shared_ptr<std::istream> override {
    return std::make_shared<std::ifstream>(filePath);
  }

  auto makeOutput() -> std::shared_ptr<std::ostream> override {
    return std::make_shared<std::ofstream>(filePath);
  }

private:
  std::string filePath;
};

class Child {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Child);
  virtual auto index() -> long = 0;
};

class Parent {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Parent);
  virtual auto index(void *) -> long = 0;
  virtual void release(void *) = 0;
};

class ParentAndChild : public Parent, public Child {
public:
  using Child::index;
  using Parent::index;
};

class WebSocketTransactionRecordObserver
    : public ObservableTransaction::Observer {
public:
  WebSocketTransactionRecordObserver(websocketpp::server<debug_custom> &server,
                                     websocketpp::connection_hdl connection,
                                     ObservableTransaction &record,
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
    return distance(
        children.begin(),
        find_if(children.begin(), children.end(),
                [a](const std::shared_ptr<WebSocketTransactionRecordObserver>
                        &child) { return child.get() == a; }));
  }

  auto index() -> long override { return parent.index(this); }

  void release(void *a) override {
    children.erase(find_if(
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

  void notifyThatHasBeenAdded(ObservableTransaction &t) override {
    children.push_back(std::make_shared<WebSocketTransactionRecordObserver>(
        server, connection, t, *this));
    nlohmann::json json;
    json["method"] = "add transaction";
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

struct AccountObserverWithName {
  std::shared_ptr<WebSocketAccountObserver> observer;
  std::string name;
};

class WebSocketModelObserver : public Budget::Observer, public Parent {
public:
  WebSocketModelObserver(websocketpp::server<debug_custom> &server,
                         websocketpp::connection_hdl connection, Budget &Budget)
      : connection{std::move(connection)}, server{server} {
    Budget.attach(this);
  }

  auto index(void *a) -> long override {
    return distance(children.begin(),
                    find_if(children.begin(), children.end(),
                            [a](const AccountObserverWithName &child) {
                              return child.observer.get() == a;
                            }));
  }

  void release(void *a) override {
    children.erase(find_if(children.begin(), children.end(),
                           [&](const AccountObserverWithName &child) -> bool {
                             return child.observer.get() == a;
                           }));
  }

  void notifyThatCategoryAllocationHasChanged(std::string_view name,
                                              USD usd) override {
    nlohmann::json json;
    json["method"] = "update account allocation";
    json["accountIndex"] = distance(
        children.begin(), find_if(children.begin(), children.end(),
                                  [name](const AccountObserverWithName &child) {
                                    return child.name == name;
                                  }));
    std::stringstream amountStream;
    amountStream << usd;
    json["amount"] = amountStream.str();
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatUnallocatedIncomeHasChanged(USD usd) override {
    nlohmann::json json;
    json["method"] = "update unallocated income";
    json["accountIndex"] = 0;
    std::stringstream amountStream;
    amountStream << usd;
    json["amount"] = amountStream.str();
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void addAccount(Account &account, std::string_view name) {
    children.push_back(
        AccountObserverWithName{std::make_shared<WebSocketAccountObserver>(
                                    server, connection, account, *this),
                                std::string{name}});
    nlohmann::json json;
    json["name"] = name;
    json["method"] = "add account";
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

  void notifyThatExpenseAccountHasBeenCreated(Account &account,
                                              std::string_view name) override {
    addAccount(account, name);
  }

  void notifyThatNetIncomeHasChanged(USD usd) override {
    nlohmann::json json;
    std::stringstream amountStream;
    amountStream << usd;
    json["amount"] = amountStream.str();
    json["method"] = "update net income";
    server.send(connection, json.dump(),
                websocketpp::frame::opcode::value::text);
  }

private:
  websocketpp::connection_hdl connection;
  websocketpp::server<debug_custom> &server;
  std::vector<AccountObserverWithName> children;
};
} // namespace

static auto backupDirectory(std::chrono::system_clock::time_point time)
    -> std::filesystem::path {
  const auto converted{std::chrono::system_clock::to_time_t(time)};
  std::stringstream backupDirectory;
  backupDirectory << std::put_time(std::localtime(&converted), "%F_%T");
  return std::filesystem::path{"/home/seth/budget/backups"} /
         backupDirectory.str();
}

namespace {
struct App {
  ObservableTransactionInMemory::Factory transactionFactory;
  InMemoryAccount incomeAccount{transactionFactory};
  InMemoryAccount::Factory accountFactory{transactionFactory};
  BudgetInMemory budget{incomeAccount, accountFactory};
  FileStreamFactory streamFactory;
  WritesTransactionToStream::Factory transactionRecordSerializationFactory;
  WritesAccountToStream::Factory accountSerializationFactory{
      transactionRecordSerializationFactory};
  WritesBudgetToStream sessionSerialization{streamFactory,
                                            accountSerializationFactory};
  ReadsTransactionFromStream::Factory transactionRecordDeserializationFactory;
  ReadsAccountFromStream::Factory accountDeserializationFactory{
      transactionRecordDeserializationFactory};
  ReadsBudgetFromStream budgetDeserialization{streamFactory,
                                              accountDeserializationFactory};
  WebSocketModelObserver webSocketNotifier;
  std::filesystem::path backupDirectory;
  std::string budgetFilePath;
  std::uintmax_t backupCount = 0;

  App(websocketpp::server<debug_custom> &server,
      websocketpp::connection_hdl connection, const std::string &budgetFilePath)
      : streamFactory{budgetFilePath}, webSocketNotifier{server,
                                                         std::move(connection),
                                                         budget},
        backupDirectory{
            budget::backupDirectory(std::chrono::system_clock::now())},
        budgetFilePath{budgetFilePath} {
    webSocketNotifier.addAccount(incomeAccount, "Income");
    budget.load(budgetDeserialization);
    std::filesystem::create_directory(backupDirectory);
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

static void call(const std::unique_ptr<App> &application,
                 const std::function<void(Budget &)> &f) {
  f(application->budget);
}

static auto methodIs(const nlohmann::json &json, std::string_view method)
    -> bool {
  return json["method"].get<std::string>() == method;
}

static auto accountName(const nlohmann::json &json) -> std::string {
  return json["name"].get<std::string>();
}

static auto accountIsMaster(const nlohmann::json &json) -> bool {
  return accountName(json) == "master";
}

static void
handleMessage(const std::unique_ptr<App> &application,
              const websocketpp::server<debug_custom>::message_ptr &message) {
  // brace-initialization seems to fail here
  const auto json = nlohmann::json::parse(message->get_payload());
  if (methodIs(json, "add transaction"))
    call(application, [&json](Budget &budget) {
      if (accountIsMaster(json))
        budget.addIncome(transaction(json));
      else
        budget.addExpense(accountName(json), transaction(json));
    });
  else if (methodIs(json, "remove transaction"))
    call(application, [&json](Budget &budget) {
      if (accountIsMaster(json))
        budget.removeIncome(transaction(json));
      else
        budget.removeExpense(accountName(json), transaction(json));
    });
  else if (methodIs(json, "verify transaction"))
    call(application, [&json](Budget &budget) {
      if (accountIsMaster(json))
        budget.verifyIncome(transaction(json));
      else
        budget.verifyExpense(accountName(json), transaction(json));
    });
  else if (methodIs(json, "transfer"))
    call(application, [&json](Budget &budget) {
      budget.transferTo(accountName(json),
                        usd(json["amount"].get<std::string>()));
    });
  else if (methodIs(json, "reduce"))
    call(application, [](Budget &budget) { budget.reduce(); });
  else if (methodIs(json, "restore"))
    call(application, [](Budget &budget) { budget.restore(); });
  else if (methodIs(json, "allocate"))
    call(application, [&json](Budget &budget) {
      budget.allocate(accountName(json),
                      usd(json["amount"].get<std::string>()));
    });
  else if (methodIs(json, "rename account"))
    call(application, [&json](Budget &budget) {
      budget.renameAccount(accountName(json),
                           json["newName"].get<std::string>());
    });
  else if (methodIs(json, "create account"))
    call(application,
         [&json](Budget &budget) { budget.createAccount(accountName(json)); });
  else if (methodIs(json, "remove account"))
    call(application,
         [&json](Budget &budget) { budget.removeAccount(accountName(json)); });
  else if (methodIs(json, "close account"))
    call(application,
         [&json](Budget &budget) { budget.closeAccount(accountName(json)); });
  else if (methodIs(json, "save")) {
    std::stringstream backupFileName;
    backupFileName << ++application->backupCount << ".txt";
    std::filesystem::copy(application->budgetFilePath,
                          application->backupDirectory / backupFileName.str());
    application->budget.save(application->sessionSerialization);
  }
}
} // namespace sbash64::budget

int main(int argc, char *argv[]) {
  if (argc < 2) {
    return EXIT_FAILURE;
  }
  const std::string budgetFilePath{argv[1]};
  std::map<void *, std::unique_ptr<sbash64::budget::App>> applications;
  websocketpp::server<debug_custom> server;
  try {
    // Set logging settings
    server.set_access_channels(websocketpp::log::alevel::all);
    server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    // Initialize ASIO
    server.init_asio();
    server.set_reuse_addr(true);

    server.set_open_handler([&server, &applications, &budgetFilePath](
                                const websocketpp::connection_hdl &connection) {
      applications[connection.lock().get()] =
          std::make_unique<sbash64::budget::App>(server, connection,
                                                 budgetFilePath);
    });

    server.set_fail_handler([&server](websocketpp::connection_hdl connection) {
      websocketpp::server<debug_custom>::connection_ptr con =
          server.get_con_from_hdl(std::move(connection));

      std::cout << "Fail handler: " << con->get_ec() << " "
                << con->get_ec().message() << std::endl;
    });

    server.set_close_handler(
        [&applications](const websocketpp::connection_hdl &connection) {
          applications.at(connection.lock().get()).reset();
        });

    server.set_message_handler(
        [&applications](
            const websocketpp::connection_hdl &connection,
            const websocketpp::server<debug_custom>::message_ptr &message) {
          sbash64::budget::handleMessage(
              applications.at(connection.lock().get()), message);
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
        con->append_header("Content-Type", "text/javascript");
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
