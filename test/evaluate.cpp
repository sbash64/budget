#include "evaluate.hpp"
#include "printer-stub.hpp"
#include "usd.hpp"
#include <functional>
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
#include <string_view>

namespace sbash64::budget::evaluate {
namespace {
class BankStub : public Model {
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

  void print(Printer &p) override { printer_ = &p; }

  auto printer() -> const Printer * { return printer_; }

  void save(PersistentMemory &p) override { persistentMemory_ = &p; }

  auto persistentMemory() -> const PersistentMemory * {
    return persistentMemory_;
  }

  auto transferredAmount() -> USD { return transferredAmount_; }

  auto accountNameTransferredTo() -> std::string {
    return accountNameTransferredTo_;
  }

  auto transferDate() -> Date { return transferDate_; }

private:
  Transaction debitedTransaction_;
  Transaction creditedTransaction_;
  std::string debitedAccountName_;
  const Printer *printer_{};
  const PersistentMemory *persistentMemory_{};
  USD transferredAmount_{};
  std::string accountNameTransferredTo_;
  Date transferDate_{};
};

class PersistentMemoryStub : public PersistentMemory {};
} // namespace

static void
testController(const std::function<void(Controller &, BankStub &, PrinterStub &,
                                        PersistentMemoryStub &)> &f) {
  Controller controller;
  BankStub bank;
  PrinterStub printer;
  PersistentMemoryStub persistentMemory;
  f(controller, bank, printer, persistentMemory);
}

static void assertPrints(testcpplite::TestResult &result,
                         std::string_view input) {
  testController([&](Controller &controller, BankStub &bank,
                     PrinterStub &printer,
                     PersistentMemoryStub &persistentMemory) {
    command(controller, bank, printer, persistentMemory, input);
    assertEqual(result, &printer, bank.printer());
  });
}

static void assertSaves(testcpplite::TestResult &result,
                        std::string_view input) {
  testController([&](Controller &controller, BankStub &bank,
                     PrinterStub &printer,
                     PersistentMemoryStub &persistentMemory) {
    command(controller, bank, printer, persistentMemory, input);
    assertEqual(result, &persistentMemory, bank.persistentMemory());
  });
}

static void assertDebitsAccount(testcpplite::TestResult &result,
                                const std::vector<std::string> &input,
                                const std::string &expectedAccountName,
                                const Transaction &expectedTransaction) {
  testController([&](Controller &controller, BankStub &bank,
                     PrinterStub &printer,
                     PersistentMemoryStub &persistentMemory) {
    for (const auto &x : input)
      command(controller, bank, printer, persistentMemory, x);
    assertEqual(result, expectedAccountName, bank.debitedAccountName());
    assertEqual(result, expectedTransaction, bank.debitedTransaction());
  });
}

static void assertCreditsAccount(testcpplite::TestResult &result,
                                 const std::vector<std::string> &input,
                                 const Transaction &expectedTransaction) {
  testController([&](Controller &controller, BankStub &bank,
                     PrinterStub &printer,
                     PersistentMemoryStub &persistentMemory) {
    for (const auto &x : input)
      command(controller, bank, printer, persistentMemory, x);
    assertEqual(result, expectedTransaction, bank.creditedTransaction());
  });
}

static void assertTransfersToAccount(testcpplite::TestResult &result,
                                     const std::vector<std::string> &input,
                                     USD expectedAmount,
                                     std::string_view expectedAccountName,
                                     const Date &expectedDate) {
  testController([&](Controller &controller, BankStub &bank,
                     PrinterStub &printer,
                     PersistentMemoryStub &persistentMemory) {
    for (const auto &x : input)
      command(controller, bank, printer, persistentMemory, x);
    assertEqual(result, expectedAmount, bank.transferredAmount());
    assertEqual(result, std::string{expectedAccountName},
                bank.accountNameTransferredTo());
    assertEqual(result, expectedDate, bank.transferDate());
  });
}

void print(testcpplite::TestResult &result) { assertPrints(result, "print"); }

void debit(testcpplite::TestResult &result) {
  assertDebitsAccount(
      result, {"debit Gifts 25", "12 27 20", "Sam's 24th"}, "Gifts",
      Transaction{2500_cents, "Sam's 24th", Date{2020, Month::December, 27}});
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

void save(testcpplite::TestResult &result) { assertSaves(result, "save"); }
} // namespace sbash64::budget::evaluate
