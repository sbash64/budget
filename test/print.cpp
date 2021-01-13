#include "print.hpp"
#include "usd.hpp"
#include <sbash64/budget/command-line.hpp>
#include <sstream>

namespace sbash64::budget::print {
namespace {
class AccountStub : public Account {
public:
  AccountStub(std::stringstream &stream, std::string name)
      : stream{stream}, name{std::move(name)} {}
  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void removeCredit(const Transaction &) override {}
  void removeDebit(const Transaction &) override {}
  void show(View &) override { stream << name; }
  void save(SessionSerialization &) override {}
  void load(SessionDeserialization &) override {}

private:
  std::stringstream &stream;
  std::string name;
};
} // namespace

static void assertFormatYields(testcpplite::TestResult &result, USD usd,
                               std::string_view expected) {
  assertEqual(result, std::string{expected}, format(usd));
}

void formatZeroDollars(testcpplite::TestResult &result) {
  assertFormatYields(result, 0_cents, "$0.00");
}

void formatOneDollar(testcpplite::TestResult &result) {
  assertFormatYields(result, 100_cents, "$1.00");
}

void formatOneCent(testcpplite::TestResult &result) {
  assertFormatYields(result, 1_cents, "$0.01");
}

void formatTenCents(testcpplite::TestResult &result) {
  assertFormatYields(result, 10_cents, "$0.10");
}

void formatNegativeDollarThirtyFour(testcpplite::TestResult &result) {
  assertFormatYields(result, -134_cents, "$-1.34");
}

void accounts(testcpplite::TestResult &result) {
  std::stringstream stream;
  StreamView printer{stream};
  AccountStub jeff{stream, "jeff"};
  AccountStub steve{stream, "steve"};
  AccountStub sue{stream, "sue"};
  AccountStub allen{stream, "allen"};
  printer.show(jeff, {&steve, &sue, &allen});
  assertEqual(result, R"(
jeff

steve

sue

allen

)",
              '\n' + stream.str() + '\n');
}

void account(testcpplite::TestResult &result) {
  std::stringstream stream;
  StreamView printer{stream};
  printer.showAccountSummary(
      "Checking", 1234_cents,
      {debit(Transaction{2500_cents, "Sam's 24th",
                         Date{2020, Month::December, 27}}),
       credit(Transaction{2734_cents, "Birthday present",
                          Date{2021, Month::October, 20}}),
       debit(Transaction{2410_cents, "Hannah's 30th",
                         Date{2021, Month::March, 18}})});
  assertEqual(result, R"(
----
Checking
$12.34

Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description
25.00                    12/27/2020          Sam's 24th
            27.34        10/20/2021          Birthday present
24.10                    03/18/2021          Hannah's 30th
----
)",
              '\n' + stream.str() + '\n');
}

void prompt(testcpplite::TestResult &result) {
  std::stringstream stream;
  StreamView printer{stream};
  printer.prompt("hello");
  assertEqual(result, "hello ", stream.str());
}

void transactionWithSuffix(testcpplite::TestResult &result) {
  std::stringstream stream;
  StreamView printer{stream};
  printer.show(Transaction{123_cents, "nope", Date{2020, Month::July, 5}},
               "hello");
  assertEqual(result, "$1.23 7/5/2020 nope hello\n", stream.str());
}
} // namespace sbash64::budget::print
