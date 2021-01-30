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
#include <utility>

namespace sbash64::budget::command_line {
namespace {
class ModelStub : public Model {
public:
  auto removedTransferAmount() -> USD { return removedTransferAmount_; }

  auto removedTransferAccountName() -> std::string {
    return removedTransferAccountName_;
  }

  auto removedTransferDate() -> Date { return removedTransferDate_; }

  auto removedDebit() -> Transaction { return removedDebit_; }

  auto removedDebitAccountName() -> std::string {
    return removedDebitAccountName_;
  }

  auto removedCredit() -> Transaction { return removedCredit_; }

  auto debitedAccountName() -> std::string { return debitedAccountName_; }

  auto debitedTransaction() -> Transaction { return debitedTransaction_; }

  auto creditedTransaction() -> Transaction { return creditedTransaction_; }

  void debit(std::string_view accountName,
             const Transaction &transaction) override {
    debitedAccountName_ = accountName;
    debitedTransaction_ = transaction;
  }

  void removeDebit(std::string_view accountName,
                   const Transaction &transaction) override {
    removedDebitAccountName_ = accountName;
    removedDebit_ = transaction;
  }

  void removeCredit(const Transaction &transaction) override {
    removedCredit_ = transaction;
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

  void removeTransfer(std::string_view accountName, USD amount,
                      Date date) override {
    removedTransferAmount_ = amount;
    removedTransferAccountName_ = accountName;
    removedTransferDate_ = date;
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

  void renameAccount(std::string_view from, std::string_view to) override {
    accountRenaming_ = from;
    accountRenamed_ = to;
  }

  void setFoundUnverifiedDebits(Transactions t) {
    foundUnverifiedDebits = std::move(t);
  }

  auto findUnverifiedDebits(std::string_view accountName, USD amount)
      -> Transactions override {
    findUnverifiedDebitsAccountName_ = accountName;
    findUnverifiedDebitsAmount_ = amount;
    return foundUnverifiedDebits;
  }

  auto verifiedDebit() -> Transaction { return verifiedDebit_; }

  void verifyDebit(std::string_view accountName,
                   const Transaction &t) override {
    verifiedDebitAccountName_ = accountName;
    verifiedDebit_ = t;
  }

  auto verifiedDebitAccountName() -> std::string {
    return verifiedDebitAccountName_;
  }

  auto findUnverifiedDebitsAccountName() -> std::string {
    return findUnverifiedDebitsAccountName_;
  }

  auto findUnverifiedDebitsAmount() -> USD {
    return findUnverifiedDebitsAmount_;
  }

private:
  Transaction verifiedDebit_;
  Transaction removedCredit_;
  Transaction removedDebit_;
  Transaction debitedTransaction_;
  Transaction creditedTransaction_;
  Date transferDate_{};
  Date removedTransferDate_{};
  Transactions foundUnverifiedDebits;
  std::string removedDebitAccountName_;
  std::string debitedAccountName_;
  std::string accountRenaming_;
  std::string accountRenamed_;
  std::string accountNameTransferredTo_;
  std::string removedTransferAccountName_;
  std::string verifiedDebitAccountName_;
  std::string findUnverifiedDebitsAccountName_;
  const View *printer_{};
  const SessionSerialization *serialization_{};
  const SessionDeserialization *deserialization_{};
  USD transferredAmount_{};
  USD removedTransferAmount_{};
  USD findUnverifiedDebitsAmount_{};
};

class SerializationStub : public SessionSerialization {
public:
  void save(Account &, const std::vector<Account *> &) override {}
  void saveAccount(std::string_view, const VerifiableTransactions &,
                   const VerifiableTransactions &) override {}
};

class DeserializationStub : public SessionDeserialization {
public:
  void loadAccount(VerifiableTransactions &,
                   VerifiableTransactions &) override {}
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

  auto enumeratedTransactions() -> Transactions {
    return enumeratedTransactions_;
  }

  void show(const Transaction &t) override { transaction_ = t; }

  void enumerate(const Transactions &t) override {
    enumeratedTransactions_ = t;
  }

  auto message() -> std::string { return message_; }

  void show(std::string_view m) override { message_ = m; }

private:
  Transaction transaction_;
  Transactions enumeratedTransactions_;
  std::string prompt_;
  std::string message_;
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
    ModelStub &model,
    const std::function<void(CommandLineInterpreter &, ModelStub &,
                             CommandLineInterfaceStub &, SerializationStub &,
                             DeserializationStub &)> &f) {
  CommandLineInterpreter interpreter;
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

static void
testController(ModelStub &model,
               const std::function<void(CommandLineInterpreter &, ModelStub &,
                                        CommandLineInterfaceStub &)> &f,
               const std::vector<std::string> &input) {
  testController(model,
                 [&](CommandLineInterpreter &interpreter, ModelStub &model_,
                     CommandLineInterfaceStub &interface, SerializationStub &,
                     DeserializationStub &) {
                   SerializationStub serialization;
                   DeserializationStub deserialization;
                   for (const auto &x : input)
                     execute(interpreter, model_, interface, serialization,
                             deserialization, x);
                   f(interpreter, model_, interface);
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
      result, {"debit", "Gifts", "25", "12 27 20", "Sam's 24th"}, "Gifts",
      Transaction{2500_cents, "Sam's 24th", Date{2020, Month::December, 27}});
}

void debitMultiWordAccount(testcpplite::TestResult &result) {
  assertDebitsAccount(
      result, {"debit", "Seth's Car Loan", "321.24", "12 27 20", "honda"},
      "Seth's Car Loan",
      Transaction{32124_cents, "honda", Date{2020, Month::December, 27}});
}

void credit(testcpplite::TestResult &result) {
  assertCreditsAccount(
      result, {"credit", "2134.35", "11 22 19", "btnrh"},
      Transaction{213435_cents, "btnrh", Date{2019, Month::November, 22}});
}

void transferTo(testcpplite::TestResult &result) {
  assertTransfersToAccount(result, {"transfer-to", "Groceries", "50", "6 3 21"},
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
          CommandLineInterfaceStub &interface) {
        assertEqual(result, "date [month day year]", interface.prompt());
      },
      {"debit", "Groceries", "40"});
}

void debitPromptsForDesriptionAfterDateEntered(
    testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, "description [anything]", interface.prompt());
      },
      {"debit", "Groceries", "40", "1 13 14"});
}

void debitShowsTransaction(testcpplite::TestResult &result) {
  assertShowsTransaction(
      result, {"debit", "Gifts", "25", "12 27 20", "Sam's 24th"}, "Gifts",
      Transaction{2500_cents, "Sam's 24th", Date{2020, Month::December, 27}});
}

void renameAccount(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &) {
        assertEqual(result, "SethsRent", model.accountRenaming());
        assertEqual(result, "Seth's Rent", model.accountRenamed());
      },
      {"rename", "SethsRent", "Seth's Rent"});
}

void renameAccountPromptsForNewName(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, "new name [anything]", interface.prompt());
      },
      {"rename", "whatever"});
}

void debitPromptsForAccountName(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, "which account? [name]", interface.prompt());
      },
      "debit");
}

void debitPromptsForAmount(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, "how much? [amount ($)]", interface.prompt());
      },
      {"debit", "Groceries"});
}

void creditPromptsForAmount(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, "how much? [amount ($)]", interface.prompt());
      },
      "credit");
}

void unrecognizedCommandPrintsMessage(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, "unknown command \"oops\"", interface.message());
      },
      "oops");
}

void removeDebit(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &) {
        assertEqual(result, "Gifts", model.removedDebitAccountName());
        assertEqual(result,
                    Transaction{2500_cents, "Sam's 24th",
                                Date{2020, Month::December, 27}},
                    model.removedDebit());
      },
      {"remove-debit", "Gifts", "25", "12 27 20", "Sam's 24th"});
}

void removeCredit(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &) {
        assertEqual(
            result,
            Transaction{200000_cents, "income", Date{2023, Month::March, 26}},
            model.removedCredit());
      },
      {"remove-credit", "2000", "3 26 23", "income"});
}

void removeTransfer(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &) {
        assertEqual(result, 50000_cents, model.removedTransferAmount());
        assertEqual(result, "Groceries", model.removedTransferAccountName());
        assertEqual(result, Date{2013, Month::July, 1},
                    model.removedTransferDate());
      },
      {"remove-transfer", "Groceries", "500", "7 1 13"});
}

void verifyDebit(testcpplite::TestResult &result) {
  ModelStub model;
  model.setFoundUnverifiedDebits(
      {{1_cents, "walmart", Date{2022, Month::January, 6}},
       {2_cents, "hyvee", Date{2023, Month::March, 26}},
       {3_cents, "sam's", Date{2021, Month::October, 2}}});
  testController(
      model,
      [&](CommandLineInterpreter &, ModelStub &model_,
          CommandLineInterfaceStub &) {
        assertEqual(result, {2_cents, "hyvee", Date{2023, Month::March, 26}},
                    model_.verifiedDebit());
        assertEqual(result, "Groceries", model_.verifiedDebitAccountName());
        assertEqual(result, "Groceries",
                    model_.findUnverifiedDebitsAccountName());
        assertEqual(result, 50000_cents, model_.findUnverifiedDebitsAmount());
      },
      {"verify-debit", "Groceries", "500", "2", "y"});
}

void verifyOnlyDebitFound(testcpplite::TestResult &result) {
  ModelStub model;
  model.setFoundUnverifiedDebits(
      {{2_cents, "hyvee", Date{2023, Month::March, 27}}});
  testController(
      model,
      [&](CommandLineInterpreter &, ModelStub &model_,
          CommandLineInterfaceStub &) {
        assertEqual(result, {2_cents, "hyvee", Date{2023, Month::March, 27}},
                    model_.verifiedDebit());
        assertEqual(result, "Groceries", model_.verifiedDebitAccountName());
        assertEqual(result, "Groceries",
                    model_.findUnverifiedDebitsAccountName());
        assertEqual(result, 50000_cents, model_.findUnverifiedDebitsAmount());
      },
      {"verify-debit", "Groceries", "500", "y"});
}

void showsDebitCandidatesForVerification(testcpplite::TestResult &result) {
  ModelStub model;
  model.setFoundUnverifiedDebits(
      {{1_cents, "walmart", Date{2022, Month::January, 6}},
       {2_cents, "hyvee", Date{2023, Month::March, 26}},
       {3_cents, "sam's", Date{2021, Month::October, 2}}});
  testController(model,
                 [&](CommandLineInterpreter &, ModelStub &,
                     CommandLineInterfaceStub &interface) {
                   assertEqual(
                       result,
                       {{1_cents, "walmart", Date{2022, Month::January, 6}},
                        {2_cents, "hyvee", Date{2023, Month::March, 26}},
                        {3_cents, "sam's", Date{2021, Month::October, 2}}},
                       interface.enumeratedTransactions());
                   assertEqual(result, "multiple candidates found - which? [n]",
                               interface.prompt());
                 },
                 {"verify-debit", "Groceries", "500"});
}

void promptsForDebitVerificationConfirmation(testcpplite::TestResult &result) {
  ModelStub model;
  model.setFoundUnverifiedDebits(
      {{1_cents, "walmart", Date{2022, Month::January, 6}},
       {2_cents, "hyvee", Date{2023, Month::March, 26}},
       {3_cents, "sam's", Date{2021, Month::October, 2}}});
  testController(
      model,
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface) {
        assertEqual(result, {2_cents, "hyvee", Date{2023, Month::March, 26}},
                    interface.transaction());
        assertEqual(result, "is the above transaction correct? [y/n]",
                    interface.prompt());
      },
      {"verify-debit", "Groceries", "500", "2"});
}
} // namespace sbash64::budget::command_line
