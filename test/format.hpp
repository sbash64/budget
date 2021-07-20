#ifndef SBASH64_BUDGET_TEST_FORMAT_HPP_
#define SBASH64_BUDGET_TEST_FORMAT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
namespace formats {
void zeroDollars(testcpplite::TestResult &);
void oneDollar(testcpplite::TestResult &);
void oneCent(testcpplite::TestResult &);
void tenCents(testcpplite::TestResult &);
void negativeOneDollarThirtyFourCents(testcpplite::TestResult &);
void negativeFifteenCents(testcpplite::TestResult &);
} // namespace formats
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
