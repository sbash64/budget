#include "command-line.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"
#include "view-stub.hpp"
#include <algorithm>
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

  void setFoundUnverifiedCredits(Transactions t) {
    foundUnverifiedCredits = std::move(t);
  }

  auto findUnverifiedDebits(std::string_view accountName, USD amount)
      -> Transactions override {
    findUnverifiedDebitsAccountName_ = accountName;
    findUnverifiedDebitsAmount_ = amount;
    return foundUnverifiedDebits;
  }

  auto findUnverifiedCredits(USD amount) -> Transactions {
    findUnverifiedCreditsAmount_ = amount;
    return foundUnverifiedCredits;
  }

  auto verifiedDebit() -> Transaction { return verifiedDebit_; }

  auto verifiedCredit() -> Transaction { return verifiedCredit_; }

  void verifyDebit(std::string_view accountName,
                   const Transaction &t) override {
    verifiedDebitAccountName_ = accountName;
    verifiedDebit_ = t;
  }

  void verifyCredit(const Transaction &t) { verifiedCredit_ = t; }

  auto verifiedDebitAccountName() -> std::string {
    return verifiedDebitAccountName_;
  }

  auto findUnverifiedDebitsAccountName() -> std::string {
    return findUnverifiedDebitsAccountName_;
  }

  auto findUnverifiedDebitsAmount() -> USD {
    return findUnverifiedDebitsAmount_;
  }

  auto findUnverifiedCreditsAmount() -> USD {
    return findUnverifiedCreditsAmount_;
  }

private:
  Transaction verifiedCredit_;
  Transaction verifiedDebit_;
  Transaction removedCredit_;
  Transaction removedDebit_;
  Transaction debitedTransaction_;
  Transaction creditedTransaction_;
  Date transferDate_{};
  Date removedTransferDate_{};
  Transactions foundUnverifiedDebits;
  Transactions foundUnverifiedCredits;
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
  USD findUnverifiedCreditsAmount_{};
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
    ModelStub &model,
    const std::function<void(CommandLineInterpreter &, ModelStub &,
                             CommandLineInterfaceStub &, SerializationStub &,
                             DeserializationStub &)> &test) {
  CommandLineInterpreter interpreter;
  CommandLineInterfaceStub interface;
  SerializationStub serialization;
  DeserializationStub deserialization;
  test(interpreter, model, interface, serialization, deserialization);
}

static void testController(
    const std::function<void(CommandLineInterpreter &, ModelStub &,
                             CommandLineInterfaceStub &, SerializationStub &,
                             DeserializationStub &)> &f) {
  ModelStub model;
  testController(model, f);
}

static void testController(
    const std::function<void(CommandLineInterpreter &, ModelStub &,
                             CommandLineInterfaceStub &, SerializationStub &,
                             DeserializationStub &)> &test,
    std::string_view input) {
  testController([=](CommandLineInterpreter &interpreter, ModelStub &model,
                     CommandLineInterfaceStub &interface,
                     SerializationStub &serialization,
                     DeserializationStub &deserialization) {
    execute(interpreter, model, interface, serialization, deserialization,
            input);
    test(interpreter, model, interface, serialization, deserialization);
  });
}

static void testController(
    const std::function<void(CommandLineInterpreter &, ModelStub &,
                             CommandLineInterfaceStub &, SerializationStub &,
                             DeserializationStub &)> &test,
    const std::vector<std::string> &input) {
  testController([&](CommandLineInterpreter &interpreter, ModelStub &model,
                     CommandLineInterfaceStub &interface,
                     SerializationStub &serialization,
                     DeserializationStub &deserialization) {
    std::for_each(input.begin(), input.end(), [&](const std::string &line) {
      execute(interpreter, model, interface, serialization, deserialization,
              line);
    });
    test(interpreter, model, interface, serialization, deserialization);
  });
}

static void
testController(ModelStub &m,
               const std::function<void(CommandLineInterpreter &, ModelStub &,
                                        CommandLineInterfaceStub &)> &test,
               const std::vector<std::string> &input) {
  testController(m, [&](CommandLineInterpreter &interpreter, ModelStub &model,
                        CommandLineInterfaceStub &interface,
                        SerializationStub &serialization,
                        DeserializationStub &deserialization) {
    std::for_each(input.begin(), input.end(), [&](const std::string &line) {
      execute(interpreter, model, interface, serialization, deserialization,
              line);
    });
    test(interpreter, model, interface);
  });
}

static void assertDebitsAccount(testcpplite::TestResult &result,
                                const std::vector<std::string> &input,
                                const std::string &expectedAccountName,
                                const Transaction &expectedTransaction) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, expectedAccountName, model.debitedAccountName());
        assertEqual(result, expectedTransaction, model.debitedTransaction());
      },
      input);
}

static void assertPrompt(testcpplite::TestResult &result,
                         CommandLineInterfaceStub &interface,
                         std::string_view s) {
  assertEqual(result, std::string{s}, interface.prompt());
}

void print(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, static_cast<const View *>(&interface),
                    model.interface());
      },
      name(Command::print));
}

void debit(testcpplite::TestResult &result) {
  assertDebitsAccount(
      result, {name(Command::debit), "Gifts", "25", "12 27 20", "Sam's 24th"},
      "Gifts",
      Transaction{2500_cents, "Sam's 24th", Date{2020, Month::December, 27}});
}

void debitMultiWordAccount(testcpplite::TestResult &result) {
  assertDebitsAccount(
      result,
      {name(Command::debit), "Seth's Car Loan", "321.24", "12 27 20", "honda"},
      "Seth's Car Loan",
      Transaction{32124_cents, "honda", Date{2020, Month::December, 27}});
}

void credit(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &) {
        assertEqual(
            result,
            Transaction{213435_cents, "btnrh", Date{2019, Month::November, 22}},
            model.creditedTransaction());
      },
      {name(Command::credit), "2134.35", "11 22 19", "btnrh"});
}

void transferTo(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, 5000_cents, model.transferredAmount());
        assertEqual(result, "Groceries", model.accountNameTransferredTo());
        assertEqual(result, Date{2021, Month::June, 3}, model.transferDate());
      },
      {name(Command::transfer), "Groceries", "50", "6 3 21"});
}

void save(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &serialization,
          DeserializationStub &) {
        assertEqual(result, &serialization, model.serialization());
      },
      name(Command::save));
}

void load(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &deserialization) {
        assertEqual(result, &deserialization, model.deserialization());
      },
      name(Command::load));
}

void debitPromptsForDate(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertPrompt(result, interface, "date [month day year]");
      },
      {name(Command::debit), "Groceries", "40"});
}

void debitPromptsForDesriptionAfterDateEntered(
    testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertPrompt(result, interface, "description [anything]");
      },
      {name(Command::debit), "Groceries", "40", "1 13 14"});
}

void debitShowsTransaction(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result,
                    Transaction{2500_cents, "Sam's 24th",
                                Date{2020, Month::December, 27}},
                    interface.transaction());
      },
      {name(Command::debit), "Gifts", "25", "12 27 20", "Sam's 24th"});
}

void renameAccount(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, "SethsRent", model.accountRenaming());
        assertEqual(result, "Seth's Rent", model.accountRenamed());
      },
      {name(Command::renameAccount), "SethsRent", "Seth's Rent"});
}

void renameAccountPromptsForNewName(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertPrompt(result, interface, "new name [anything]");
      },
      {name(Command::renameAccount), "whatever"});
}

void debitPromptsForAccountName(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertPrompt(result, interface, "which account? [name]");
      },
      name(Command::debit));
}

void debitPromptsForAmount(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertPrompt(result, interface, "how much? [amount ($)]");
      },
      {name(Command::debit), "Groceries"});
}

void creditPromptsForAmount(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertPrompt(result, interface, "how much? [amount ($)]");
      },
      name(Command::credit));
}

void unrecognizedCommandPrintsMessage(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &,
          CommandLineInterfaceStub &interface, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, "unknown command \"oops\"", interface.message());
      },
      "oops");
}

void removeDebit(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, "Gifts", model.removedDebitAccountName());
        assertEqual(result,
                    Transaction{2500_cents, "Sam's 24th",
                                Date{2020, Month::December, 27}},
                    model.removedDebit());
      },
      {name(Command::removeDebit), "Gifts", "25", "12 27 20", "Sam's 24th"});
}

void removeCredit(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &) {
        assertEqual(
            result,
            Transaction{200000_cents, "income", Date{2023, Month::March, 26}},
            model.removedCredit());
      },
      {name(Command::removeCredit), "2000", "3 26 23", "income"});
}

void removeTransfer(testcpplite::TestResult &result) {
  testController(
      [&](CommandLineInterpreter &, ModelStub &model,
          CommandLineInterfaceStub &, SerializationStub &,
          DeserializationStub &) {
        assertEqual(result, 50000_cents, model.removedTransferAmount());
        assertEqual(result, "Groceries", model.removedTransferAccountName());
        assertEqual(result, Date{2013, Month::July, 1},
                    model.removedTransferDate());
      },
      {name(Command::removeTransfer), "Groceries", "500", "7 1 13"});
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
      {name(Command::verifyDebit), "Groceries", "500", "2", "y"});
}

void verifyCredit(testcpplite::TestResult &result) {
  ModelStub model;
  model.setFoundUnverifiedCredits(
      {{1_cents, "gift", Date{2022, Month::January, 6}},
       {2_cents, "paycheck", Date{2023, Month::March, 26}},
       {3_cents, "found on the ground", Date{2021, Month::October, 2}}});
  testController(
      model,
      [&](CommandLineInterpreter &, ModelStub &model_,
          CommandLineInterfaceStub &) {
        assertEqual(result, {2_cents, "paycheck", Date{2023, Month::March, 26}},
                    model_.verifiedCredit());
        assertEqual(result, 50000_cents, model_.findUnverifiedCreditsAmount());
      },
      {name(Command::verifyCredit), "500", "2", "y"});
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
      {name(Command::verifyDebit), "Groceries", "500", "y"});
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
                 {name(Command::verifyDebit), "Groceries", "500"});
}

void showsDebitCandidatesForVerificationAgainWhenSelectedDebitIsOutOfRange(
    testcpplite::TestResult &result) {
  ModelStub model;
  model.setFoundUnverifiedDebits(
      {{1_cents, "walmart", Date{2022, Month::January, 6}},
       {2_cents, "hyvee", Date{2023, Month::March, 26}},
       {3_cents, "sam's", Date{2021, Month::October, 2}}});
  testController(model,
                 [&](CommandLineInterpreter &, ModelStub &,
                     CommandLineInterfaceStub &interface) {
                   assertEqual(result, "try again - which? [n]",
                               interface.prompt());
                 },
                 {name(Command::verifyDebit), "Groceries", "500", "4"});
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
      {name(Command::verifyDebit), "Groceries", "500", "2"});
}

void showsMessageWhenNoCandidatesFoundForVerification(
    testcpplite::TestResult &result) {
  ModelStub model;
  model.setFoundUnverifiedDebits({});
  testController(model,
                 [&](CommandLineInterpreter &, ModelStub &,
                     CommandLineInterfaceStub &interface) {
                   assertEqual(result, "no transactions matching amount found!",
                               interface.message());
                 },
                 {name(Command::verifyDebit), "Groceries", "500"});
}
} // namespace sbash64::budget::command_line
