#include "print.hpp"
#include "usd.hpp"
#include <functional>
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
  void rename(std::string_view) override {}
  auto findUnverifiedDebits(USD) -> Transactions override { return {}; }
  auto findUnverifiedCredits(USD) -> Transactions override { return {}; }
  void verifyDebit(const Transaction &) override {}
  void verifyCredit(const Transaction &) override {}

private:
  std::stringstream &stream;
  std::string name;
};
} // namespace

static void testCommandLineStream(
    const std::function<void(CommandLineStream &, std::stringstream &)> &f) {
  std::stringstream stream;
  CommandLineStream commandLineStream{stream};
  f(commandLineStream, stream);
}

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
  testCommandLineStream(
      [&](CommandLineStream &commandLineStream, std::stringstream &stream) {
        AccountStub jeff{stream, "jeff"};
        AccountStub steve{stream, "steve"};
        AccountStub sue{stream, "sue"};
        AccountStub allen{stream, "allen"};
        commandLineStream.show(jeff, {&steve, &sue, &allen});
        assertEqual(result, R"(
jeff

steve

sue

allen

)",
                    '\n' + stream.str() + '\n');
      });
}

void account(testcpplite::TestResult &result) {
  testCommandLineStream(
      [&](CommandLineStream &commandLineStream, std::stringstream &stream) {
        commandLineStream.showAccountSummary(
            "Checking", 1234_cents,
            {debit(Transaction{2500_cents, "Sam's 24th",
                               Date{2020, Month::December, 27}}),
             credit(Transaction{2734_cents, "Birthday present",
                                Date{2021, Month::October, 20}}),
             verifiedDebit(Transaction{2410_cents, "Hannah's 30th",
                                       Date{2021, Month::March, 8}})});
        assertEqual(result, R"(
----
Checking
$12.34

Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description
   *25.00                12/27/2020          Sam's 24th
                *27.34   10/20/2021          Birthday present
    24.10                03/08/2021          Hannah's 30th
----
)",
                    '\n' + stream.str() + '\n');
      });
}

void prompt(testcpplite::TestResult &result) {
  std::stringstream stream;
  CommandLineStream commandLineStream{stream};
  commandLineStream.prompt("hello");
  assertEqual(result, "hello ", stream.str());
}

void transactionWithSuffix(testcpplite::TestResult &result) {
  testCommandLineStream(
      [&](CommandLineStream &commandLineStream, std::stringstream &stream) {
        commandLineStream.show(
            Transaction{123_cents, "nope", Date{2020, Month::July, 5}});
        assertEqual(result, "$1.23 7/5/2020 nope\n", stream.str());
      });
}

void message(testcpplite::TestResult &result) {
  testCommandLineStream(
      [&](CommandLineStream &commandLineStream, std::stringstream &stream) {
        commandLineStream.show("hello");
        assertEqual(result, "hello\n", stream.str());
      });
}

void enumeratedTransactions(testcpplite::TestResult &result) {
  testCommandLineStream(
      [&](CommandLineStream &commandLineStream, std::stringstream &stream) {
        commandLineStream.enumerate(
            {{1_cents, "walmart", Date{2022, Month::January, 6}},
             {2_cents, "hyvee", Date{2023, Month::March, 26}},
             {3_cents, "sam's", Date{2021, Month::October, 2}}});
        assertEqual(result,
                    R"(
[1] $0.01 1/6/2022 walmart
[2] $0.02 3/26/2023 hyvee
[3] $0.03 10/2/2021 sam's
)",
                    '\n' + stream.str());
      });
}
} // namespace sbash64::budget::print
