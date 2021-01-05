#include "evaluate.hpp"
#include "usd.hpp"
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
#include <string_view>

namespace sbash64::budget::evaluate {
namespace {
class PrinterStub : public Printer {};

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
  USD transferredAmount_{};
  std::string accountNameTransferredTo_;
  Date transferDate_;
};
} // namespace

static void assertPrints(testcpplite::TestResult &result,
                         std::string_view input) {
  Controller controller;
  BankStub bank;
  PrinterStub printer;
  command(controller, bank, printer, input);
  assertEqual(result, &printer, bank.printer());
}

static void assertDebitsAccount(testcpplite::TestResult &result,
                                const std::vector<std::string> &input,
                                const std::string &expectedAccountName,
                                const Transaction &expectedTransaction) {
  Controller controller;
  BankStub bank;
  PrinterStub printer;
  for (const auto &x : input)
    command(controller, bank, printer, x);
  assertEqual(result, expectedAccountName, bank.debitedAccountName());
  assertEqual(result, expectedTransaction, bank.debitedTransaction());
}

static void assertCreditsAccount(testcpplite::TestResult &result,
                                 const std::vector<std::string> &input,
                                 const Transaction &expectedTransaction) {
  Controller controller;
  BankStub bank;
  PrinterStub printer;
  for (const auto &x : input)
    command(controller, bank, printer, x);
  assertEqual(result, expectedTransaction, bank.creditedTransaction());
}

static void assertTransfersToAccount(testcpplite::TestResult &result,
                                     const std::vector<std::string> &input,
                                     USD expectedAmount,
                                     std::string_view expectedAccountName,
                                     const Date &expectedDate) {
  Controller controller;
  BankStub bank;
  PrinterStub printer;
  for (const auto &x : input)
    command(controller, bank, printer, x);
  assertEqual(result, expectedAmount, bank.transferredAmount());
  assertEqual(result, std::string{expectedAccountName},
              bank.accountNameTransferredTo());
  assertEqual(result, expectedDate, bank.transferDate());
}

void printCommand(testcpplite::TestResult &result) {
  assertPrints(result, "print");
}

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

void transferToCommand(testcpplite::TestResult &result) {
  assertTransfersToAccount(result, {"transferto Groceries 50", "6 3 21"},
                           5000_cents, "Groceries", Date{2021, Month::June, 3});
}
} // namespace sbash64::budget::evaluate
