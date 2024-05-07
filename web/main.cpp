#include <sbash64/budget/account.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/parse.hpp>
#include <sbash64/budget/presentation.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sbash64/budget/transaction.hpp>

#include <nlohmann/json.hpp>

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/logger/levels.hpp>
#include <websocketpp/server.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

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

void assignMethod(nlohmann::json &json, std::string_view name) {
  json["method"] = name;
}

void assignAccountIndex(nlohmann::json &json, gsl::index i) {
  json["accountIndex"] = i;
}

void assignAmount(nlohmann::json &json, std::string_view x) {
  json["amount"] = x;
}

void assignTransactionIndex(nlohmann::json &json, gsl::index i) {
  json["transactionIndex"] = i;
}

void send(websocketpp::server<websocketpp::config::asio> &server,
          websocketpp::connection_hdl connection, const nlohmann::json &json) {
  server.send(std::move(connection), json.dump(),
              websocketpp::frame::opcode::value::text);
}

class BrowserView : public View {
public:
  BrowserView(websocketpp::server<websocketpp::config::asio> &server,
              websocketpp::connection_hdl connection)
      : connection{std::move(connection)}, server{server} {}

  void updateAccountAllocation(gsl::index accountIndex,
                               std::string_view s) override {
    nlohmann::json json;
    assignMethod(json, "update account allocation");
    assignAccountIndex(json, accountIndex);
    assignAmount(json, s);
    send(server, connection, json);
  }

  void updateAccountBalance(gsl::index accountIndex,
                            std::string_view s) override {
    nlohmann::json json;
    assignMethod(json, "update account balance");
    assignAccountIndex(json, accountIndex);
    assignAmount(json, s);
    send(server, connection, json);
  }

  void putCheckmarkNextToTransactionRow(gsl::index accountIndex,
                                        gsl::index index) override {
    nlohmann::json json;
    assignMethod(json, "check transaction row");
    assignAccountIndex(json, accountIndex);
    assignTransactionIndex(json, index);
    send(server, connection, json);
  }

  void deleteTransactionRow(gsl::index accountIndex,
                            gsl::index index) override {
    nlohmann::json json;
    assignMethod(json, "delete transaction row");
    assignAccountIndex(json, accountIndex);
    assignTransactionIndex(json, index);
    send(server, connection, json);
  }

  void addTransactionRow(gsl::index accountIndex, std::string_view amount,
                         std::string_view date, std::string_view description,
                         gsl::index index) override {
    nlohmann::json json;
    assignMethod(json, "add transaction row");
    assignAccountIndex(json, accountIndex);
    assignTransactionIndex(json, index);
    json["description"] = description;
    assignAmount(json, amount);
    json["date"] = date;
    send(server, connection, json);
  }

  void removeTransactionRowSelection(gsl::index accountIndex,
                                     gsl::index index) override {
    nlohmann::json json;
    assignMethod(json, "remove transaction row selection");
    assignAccountIndex(json, accountIndex);
    assignTransactionIndex(json, index);
    send(server, connection, json);
  }

  void addNewAccountTable(std::string_view name, gsl::index index) override {
    nlohmann::json json;
    assignMethod(json, "add account table");
    json["name"] = name;
    assignAccountIndex(json, index);
    send(server, connection, json);
  }

  void deleteAccountTable(gsl::index index) override {
    nlohmann::json json;
    assignMethod(json, "delete account table");
    assignAccountIndex(json, index);
    send(server, connection, json);
  }

  void updateNetIncome(std::string_view s) override {
    nlohmann::json json;
    assignMethod(json, "update net income");
    assignAmount(json, s);
    send(server, connection, json);
  }

  void markAsSaved() override {
    nlohmann::json json;
    assignMethod(json, "mark as saved");
    send(server, connection, json);
  }

  void markAsUnsaved() override {
    nlohmann::json json;
    assignMethod(json, "mark as unsaved");
    send(server, connection, json);
  }

private:
  websocketpp::connection_hdl connection;
  websocketpp::server<websocketpp::config::asio> &server;
};
} // namespace

static auto backupDirectory(const std::filesystem::path &parentPath,
                            std::chrono::system_clock::time_point time)
    -> std::filesystem::path {
  const auto converted{std::chrono::system_clock::to_time_t(time)};
  std::stringstream backupDirectory;
  backupDirectory << std::put_time(std::localtime(&converted), "%F_%T");
  return parentPath / backupDirectory.str();
}

namespace {
struct App {
  ObservableTransactionInMemory::Factory transactionFactory;
  AccountInMemory incomeAccount{transactionFactory};
  AccountInMemory::Factory accountFactory{transactionFactory};
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
  BrowserView browserView;
  BudgetPresenter presenter;
  std::filesystem::path backupDirectory;
  std::string budgetFilePath;
  std::uintmax_t backupCount = 0;

  App(websocketpp::server<websocketpp::config::asio> &server,
      websocketpp::connection_hdl connection, const std::string &budgetFilePath,
      const std::filesystem::path &backupParentPath)
      : streamFactory{budgetFilePath},
        browserView{server, std::move(connection)}, presenter{incomeAccount},
        backupDirectory{budget::backupDirectory(
            backupParentPath, std::chrono::system_clock::now())},
        budgetFilePath{budgetFilePath} {
    budget.attach(&presenter);
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

static auto methodIs(const nlohmann::json &json, std::string_view method)
    -> bool {
  return json["method"].get<std::string>() == method;
}

static auto accountName(const nlohmann::json &json) -> std::string {
  return json["name"].get<std::string>();
}

static auto accountIsIncome(const nlohmann::json &json) -> bool {
  return accountName(json) == "Income";
}

static void
handleMessage(Budget &budget, std::uintmax_t &backupCount,
              std::string_view budgetFilePath,
              const std::filesystem::path &backupDirectory,
              WritesBudgetToStream &sessionSerialization,
              const websocketpp::server<websocketpp::config::asio>::message_ptr
                  &message) {
  // brace-initialization seems to fail here
  const auto json = nlohmann::json::parse(message->get_payload());
  if (methodIs(json, "add transaction"))
    if (accountIsIncome(json))
      budget.addIncome(transaction(json));
    else
      budget.addExpense(accountName(json), transaction(json));
  else if (methodIs(json, "remove transaction"))
    if (accountIsIncome(json))
      budget.removeIncome(transaction(json));
    else
      budget.removeExpense(accountName(json), transaction(json));
  else if (methodIs(json, "verify transaction"))
    if (accountIsIncome(json))
      budget.verifyIncome(transaction(json));
    else
      budget.verifyExpense(accountName(json), transaction(json));
  else if (methodIs(json, "transfer"))
    budget.transferTo(accountName(json),
                      usd(json["amount"].get<std::string>()));
  else if (methodIs(json, "reduce"))
    budget.reduce();
  else if (methodIs(json, "restore"))
    budget.restore();
  else if (methodIs(json, "allocate"))
    budget.allocate(accountName(json), usd(json["amount"].get<std::string>()));
  else if (methodIs(json, "rename account"))
    budget.renameAccount(accountName(json), json["newName"].get<std::string>());
  else if (methodIs(json, "create account"))
    budget.createAccount(accountName(json));
  else if (methodIs(json, "remove account"))
    budget.removeAccount(accountName(json));
  else if (methodIs(json, "close account"))
    budget.closeAccount(accountName(json));
  else if (methodIs(json, "save")) {
    std::stringstream backupFileName;
    backupFileName << ++backupCount << ".txt";
    std::filesystem::copy(budgetFilePath,
                          backupDirectory / backupFileName.str());
    budget.save(sessionSerialization);
  }
}
} // namespace sbash64::budget

int main(int argc, char *argv[]) {
  if (argc < 4) {
    return EXIT_FAILURE;
  }
  const std::string budgetFilePath{argv[1]};
  const std::filesystem::path backupParentPath{argv[2]};
  const auto port{std::stoi(argv[3])};

  sbash64::budget::ObservableTransactionInMemory::Factory transactionFactory;
  sbash64::budget::AccountInMemory incomeAccount{transactionFactory};
  sbash64::budget::AccountInMemory::Factory accountFactory{transactionFactory};
  sbash64::budget::BudgetInMemory budget{incomeAccount, accountFactory};
  sbash64::budget::FileStreamFactory streamFactory{budgetFilePath};
  sbash64::budget::WritesTransactionToStream::Factory
      transactionRecordSerializationFactory;
  sbash64::budget::WritesAccountToStream::Factory accountSerializationFactory{
      transactionRecordSerializationFactory};
  sbash64::budget::WritesBudgetToStream sessionSerialization{
      streamFactory, accountSerializationFactory};
  sbash64::budget::ReadsTransactionFromStream::Factory
      transactionRecordDeserializationFactory;
  sbash64::budget::ReadsAccountFromStream::Factory
      accountDeserializationFactory{transactionRecordDeserializationFactory};
  sbash64::budget::ReadsBudgetFromStream budgetDeserialization{
      streamFactory, accountDeserializationFactory};
  sbash64::budget::BudgetPresenter presenter{incomeAccount};
  std::filesystem::path backupDirectory{sbash64::budget::backupDirectory(
      backupParentPath, std::chrono::system_clock::now())};
  std::uintmax_t backupCount = 0;
  budget.attach(&presenter);
  budget.load(budgetDeserialization);
  std::filesystem::create_directory(backupDirectory);

  std::map<void *, std::unique_ptr<sbash64::budget::BrowserView>> views;
  std::mutex budgetMutex;

  websocketpp::server<websocketpp::config::asio> server;
  server.clear_access_channels(websocketpp::log::alevel::all);
  server.set_access_channels(websocketpp::log::alevel::access_core);
  try {
    server.init_asio();
    server.set_open_handler([&server, &presenter, &views, &budgetMutex](
                                const websocketpp::connection_hdl &connection) {
      std::lock_guard lock{budgetMutex};
      auto view =
          std::make_unique<sbash64::budget::BrowserView>(server, connection);
      presenter.add(view.get());
      views[connection.lock().get()] = std::move(view);
    });
    server.set_fail_handler([&server](websocketpp::connection_hdl connection) {
      const auto con = server.get_con_from_hdl(std::move(connection));
      std::cout << "Fail handler: " << con->get_ec() << " "
                << con->get_ec().message() << '\n';
    });
    server.set_close_handler(
        [&views, &presenter,
         &budgetMutex](const websocketpp::connection_hdl &connection) {
          std::lock_guard lock{budgetMutex};
          auto node{views.extract(connection.lock().get())};
          presenter.remove(node.mapped().get());
        });
    server.set_message_handler(
        [&budget, &backupCount, &budgetFilePath, &backupDirectory,
         &sessionSerialization, &budgetMutex](
            const websocketpp::connection_hdl &,
            const websocketpp::server<websocketpp::config::asio>::message_ptr
                &message) {
          std::lock_guard lock{budgetMutex};
          sbash64::budget::handleMessage(budget, backupCount, budgetFilePath,
                                         backupDirectory, sessionSerialization,
                                         message);
        });
    server.set_http_handler([&server](websocketpp::connection_hdl connection) {
      const auto con = server.get_con_from_hdl(std::move(connection));
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
    server.listen(port);
    server.start_accept();
    std::cout << "Listening on port " << port << "..." << '\n';
    server.run();
  } catch (websocketpp::exception const &e) {
    std::cout << e.what() << '\n';
  } catch (const std::exception &e) {
    std::cout << e.what() << '\n';
  } catch (...) {
    std::cout << "other exception" << '\n';
  }
}
