#ifndef SBASH64_BUDGET_TEST_PRINT_HPP_
#define SBASH64_BUDGET_TEST_PRINT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64 {
namespace budget {
namespace print {
void prettyBudgetHavingNoIncomeNorExpenses(testcpplite::TestResult &);
void formatZeroDollars(testcpplite::TestResult &);
void formatOneDollar(testcpplite::TestResult &);
void formatOneCent(testcpplite::TestResult &);
void formatTenCents(testcpplite::TestResult &);
void prettyBudgetHavingNoExpenses(testcpplite::TestResult &);
} // namespace print
} // namespace budget
} // namespace sbash64

#endif
