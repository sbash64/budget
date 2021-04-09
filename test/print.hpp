#ifndef SBASH64_BUDGET_TEST_PRINT_HPP_
#define SBASH64_BUDGET_TEST_PRINT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
namespace test::format {
void zeroDollars(testcpplite::TestResult &);
void oneDollar(testcpplite::TestResult &);
void oneCent(testcpplite::TestResult &);
void tenCents(testcpplite::TestResult &);
void negativeDollarThirtyFour(testcpplite::TestResult &);
void negativeFifteen(testcpplite::TestResult &);
} // namespace test::format
namespace print {
void accounts(testcpplite::TestResult &);
void account(testcpplite::TestResult &);
void prompt(testcpplite::TestResult &);
void transactionWithSuffix(testcpplite::TestResult &);
void message(testcpplite::TestResult &);
void enumeratedTransactions(testcpplite::TestResult &);
} // namespace print
} // namespace sbash64::budget

#endif
