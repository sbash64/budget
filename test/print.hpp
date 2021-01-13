#ifndef SBASH64_BUDGET_TEST_PRINT_HPP_
#define SBASH64_BUDGET_TEST_PRINT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::print {
void formatZeroDollars(testcpplite::TestResult &);
void formatOneDollar(testcpplite::TestResult &);
void formatOneCent(testcpplite::TestResult &);
void formatTenCents(testcpplite::TestResult &);
void formatNegativeDollarThirtyFour(testcpplite::TestResult &);
void accounts(testcpplite::TestResult &);
void account(testcpplite::TestResult &);
void prompt(testcpplite::TestResult &);
void transactionWithSuffix(testcpplite::TestResult &);
} // namespace sbash64::budget::print

#endif
