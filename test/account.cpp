#include "account.hpp"
#include "usd.hpp"
#include <sbash64/budget/account.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
namespace {
class PrinterStub : public Printer {
public:
  auto accountBalance() -> USD { return accountBalance_; }

  auto accountTransactions() -> std::vector<PrintableTransaction> {
    return accountTransactions_;
  }

  void printAccountSummary(
      USD balance,
      const std::vector<PrintableTransaction> &transactions) override {
    accountTransactions_ = transactions;
    accountBalance_ = balance;
  }

private:
  std::vector<PrintableTransaction> accountTransactions_;
  USD accountBalance_{};
};
} // namespace

constexpr auto to_integral(Transaction::Type e) ->
    typename std::underlying_type<Transaction::Type>::type {
  return static_cast<typename std::underlying_type<Transaction::Type>::type>(e);
}

static void assertEqual(testcpplite::TestResult &result,
                        const PrintableTransaction &expected,
                        const PrintableTransaction &actual) {
  assertEqual(result, expected.transaction, actual.transaction);
  assertEqual(result, to_integral(expected.type), to_integral(actual.type));
}

void printPrintsAllTransactionsInChronologicalOrderAndBalance(
    testcpplite::TestResult &result) {
  InMemoryAccount account;
  account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  account.debit(
      Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  account.credit(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  PrinterStub printer;
  account.print(printer);
  assertEqual(result, 123_cents - 456_cents + 789_cents,
              printer.accountBalance());
  assertEqual(result,
              printableDebit(Transaction{456_cents, "gorilla",
                                         Date{2020, Month::January, 20}}),
              printer.accountTransactions().at(0));
  assertEqual(result,
              printableCredit(Transaction{789_cents, "chimpanzee",
                                          Date{2020, Month::June, 1}}),
              printer.accountTransactions().at(1));
  assertEqual(result,
              printableCredit(
                  Transaction{123_cents, "ape", Date{2020, Month::June, 2}}),
              printer.accountTransactions().at(2));
}
} // namespace sbash64::budget::account