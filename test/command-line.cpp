#include "command-line.hpp"
#include "persistent-memory-stub.hpp"
#include "sbash64/budget/budget.hpp"
#include "usd.hpp"
#include "view-stub.hpp"
#include <functional>
#include <sbash64/budget/command-line.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
#include <string_view>

namespace sbash64::budget::command_line {
namespace {
class ModelStub : public Model {
public:
  auto debitedAccountName() -> std::string { return debitedAccountName_; }

  auto debitedTransaction() -> Transaction { return debitedTransaction_; }

  auto creditedTransaction() -> Transaction { return creditedTransaction_; }

  void debit(std::string_view accountName,
             const Transaction &transaction) override {
    debitedAccountName_ = accountName;
    debitedTransaction_ = transaction;
  }

  void credit(const Transaction &transaction) override {
    creditedTransaction_ = transaction;
  }

  void transferTo(std::string_view accountName, USD amount,
                  Date date) override {
    transferredAmount_ = amount;
    accountNameTransferredTo_ = accountName;
    transferDate_ = date;
  }

  void show(View &p) override { printer_ = &p; }

  auto interface() -> const View * { return printer_; }

  void save(SessionSerialization &s) override { serialization_ = &s; }

  auto serialization() -> const SessionSerialization * {
    return serialization_;
  }

  void load(SessionDeserialization &s) override { deserialization_ = &s; }

  auto deserialization() -> const SessionDeserialization * {
    return deserialization_;
  }

  auto transferredAmount() -> USD { return transferredAmount_; }

  auto accountNameTransferredTo() -> std::string {
    return accountNameTransferredTo_;
  }

  auto transferDate() -> Date { return transferDate_; }

  auto accountRenaming() -> std::string { return accountRenaming_; }

  auto accountRenamed() -> std::string { return accountRenamed_; }

  void renameAccount(std::string_view from, std::string_view to) {
    accountRenaming_ = from;
    accountRenamed_ = to;
  }

private:
  Transaction debitedTransaction_;
  Transaction creditedTransaction_;
  Date transferDate_{};
  std::string debitedAccountName_;
  std::string accountRenaming_;
  std::string accountRenamed_;
  std::string accountNameTransferredTo_;
  const View *printer_{};
  const SessionSerialization *serialization_{};
  const SessionDeserialization *deserialization_{};
  USD transferredAmount_{};
};

class SerializationStub : public SessionSerialization {
public:
  void save(Account &, const std::vector<Account *> &) override {}
  void saveAccount(std::string_view, const std::vector<Transaction> &,
                   const std::vector<Transaction> &) override {}
};

class DeserializationStub : public SessionDeserialization {
public:
  void loadAccount(std::vector<Transaction> &,
                   std::vector<Transaction> &) override {}
  void load(
      Account::Factory &, std::shared_ptr<Account> &,
      std::map<std::string, std::shared_ptr<Account>, std::less<>> &) override {
  }
};

class CommandLineInterfaceStub : public ViewStub, public CommandLineInterface {
public:
  auto prompt() -> std::string { return prompt_; }

  void prompt(std::string_view s) override { prompt_ = s; }

  auto transaction() -> Transaction { return transaction_; }

  auto transactionSuffix() -> std::string { return transactionSuffix_; }

  void show(const Transaction &t, std::string_view suffix) override {
    transaction_ = t;
    transactionSuffix_ = suffix;
  }

private:
  Transaction transaction_;
  std::string prompt_;
  std::string transactionSuffix_;
};
} // namespace

static void testController(
    const std::function<void(CommandLineInterpreter &, ModelStub &,
                             CommandLineInterfaceStub &, SerializationStub &,
                             DeserializationStub &)> &f) {
  CommandLineInterpreter interpreter;
  ModelStub model;
  CommandLineInterfaceStub interface;
  SerializationStub serialization;
  DeserializationStub deserialization;
  f(interpreter, model, interface, serialization, deserialization);
}

static void testController(
    const std::function<void(CommandLineInterpreter &, ModelStub &,
                             CommandLineInterfaceStub &, SerializationStub &,
                             DeserializationStub &)> &f,
    std::string_view input) {
  testController([&](CommandLineInterpreter &interpreter, ModelStub &model,
                     CommandLineInterfaceStub &interface,
                     SerializationStub &serialization,
                     DeserializationStub &deserialization) {
    execute(interpreter, model, interface, serialization, deserialization,
            input);
    f(interpreter, model, interface, serialization, deserialization);
  });
}

static void
testController(const std::function<void(CommandLineInterpreter &, ModelStub &,
                                        CommandLineInterfaceStub &)> &f,
               std::string_view input) {
  testController([&](CommandLineInterpreter &interpreter, ModelStub &model,
                     CommandLineInterfaceStub &interface, SerializationStub &,
                     DeserializationStub &) {
    SerializationStub serialization;
    DeserializationStub deserialization;
    execute(interpreter, model, interface, serialization, deserialization,
            input);
    f(interpreter, model, interface);
  });
}

static void
testController(const std::function<void(CommandLineInterpreter &, ModelStub &,
                                        CommandLineInterfaceStub &)> &f,
               const std::vector<std::string> &input) {
  testController([&](CommandLineInterpreter &interpreter, ModelStub &model,
                     CommandLineInterfaceStub &interface, SerializationStub &,
                     DeserializationStub &) {
    SerializationStub serialization;
    DeserializationStub deserialization;
    for (const auto &x : input)
      execute(interpreter, model, interface, serialization, deserialization, x);
    f(interpreter, model, interface);
  });
}

static void assertPrints(testcpplite::TestResult &result,
                         std::string_view input) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, &interface, model.interface());
      },
      input);
}

static void assertDebitsAccount(testcpplite::TestResult &result,
                                const std::vector<std::string> &input,
                                const std::string &expectedAccountName,
                                const Transaction &expectedTransaction) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &) {
        assertEqual(result, expectedAccountName, model.debitedAccountName());
        assertEqual(result, expectedTransaction, model.debitedTransaction());
      },
      input);
}

static void assertShowsTransaction(testcpplite::TestResult &result,
                                   const std::vector<std::string> &input,
                                   const std::string &expectedAccountName,
                                   const Transaction &expectedTransaction) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, expectedTransaction, interface.transaction());
        assertEqual(result, "-> " + expectedAccountName,
                    interface.transactionSuffix());
      },
      input);
}

static void assertCreditsAccount(testcpplite::TestResult &result,
                                 const std::vector<std::string> &input,
                                 const Transaction &expectedTransaction) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &bank,
          CommandLineInterfaceStub &) {
        assertEqual(result, expectedTransaction, bank.creditedTransaction());
      },
      input);
}

static void assertTransfersToAccount(testcpplite::TestResult &result,
                                     const std::vector<std::string> &input,
                                     USD expectedAmount,
                                     std::string_view expectedAccountName,
                                     const Date &expectedDate) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &) {
        assertEqual(result, expectedAmount, model.transferredAmount());
        assertEqual(result, std::string{expectedAccountName},
                    model.accountNameTransferredTo());
        assertEqual(result, expectedDate, model.transferDate());
      },
      input);
}

void print(testcpplite::TestResult &result) { assertPrints(result, "print"); }

void debit(testcpplite::TestResult &result) {
  assertDebitsAccount(
      result, {"debit Gifts 25", "12 27 20", "Sam's 24th"}, "Gifts",
      Transaction{2500_cents, "Sam's 24th", Date{2020, Month::December, 27}});
}

void debitMultiWordAccount(testcpplite::TestResult &result) {
  assertDebitsAccount(
      result, {"debit Seth's Car Loan 321.24", "12 27 20", "honda"},
      "Seth's Car Loan",
      Transaction{32124_cents, "honda", Date{2020, Month::December, 27}});
}

void credit(testcpplite::TestResult &result) {
  assertCreditsAccount(
      result, {"credit 2134.35", "11 22 19", "btnrh"},
      Transaction{213435_cents, "btnrh", Date{2019, Month::November, 22}});
}

void transferTo(testcpplite::TestResult &result) {
  assertTransfersToAccount(result, {"transferto Groceries 50", "6 3 21"},
                           5000_cents, "Groceries", Date{2021, Month::June, 3});
}

void save(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &serialization,
          DeserializationStub &) {
        assertEqual(result, &serialization, model.serialization());
      },
      "save");
}

void load(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &deserialization) {
        assertEqual(result, &deserialization, model.deserialization());
      },
      "load");
}

void debitPromptsForDate(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, "date [month day year]", interface.prompt());
      },
      "debit Groceries 40");
}

void debitPromptsForDesriptionAfterDateEntered(
    testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, "description [anything]", interface.prompt());
      },
      {"debit Groceries 40", "1 13 14"});
}

void debitShowsTransaction(testcpplite::TestResult &result) {
  assertShowsTransaction(
      result, {"debit Gifts 25", "12 27 20", "Sam's 24th"}, "Gifts",
      Transaction{2500_cents, "Sam's 24th", Date{2020, Month::December, 27}});
}

void renameAccount(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &) {
        assertEqual(result, "SethsRent", model.accountRenaming());
        assertEqual(result, "Seth's Rent", model.accountRenamed());
      },
      {"rename SethsRent", "Seth's Rent"});
}

void renameAccountPromptsForNewName(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, "new name [anything]", interface.prompt());
      },
      "rename whatever");
}
} // namespace sbash64::budget::command_line
