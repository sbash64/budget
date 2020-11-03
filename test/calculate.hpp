#ifndef SBASH64_BUDGET_TEST_CALCULATE_HPP_
#define SBASH64_BUDGET_TEST_CALCULATE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64 {
namespace budget {
namespace calculate {
void differenceHavingNoIncomeNorExpenses(testcpplite::TestResult &);
void differenceHavingIncomeButNoExpenses(testcpplite::TestResult &);
void differenceHavingOneExpense(testcpplite::TestResult &);
void differenceHavingTwoExpenses(testcpplite::TestResult &);
void categoryTotalHavingNoExpenses(testcpplite::TestResult &);
void categoryTotalHavingOneUnrelatedExpense(testcpplite::TestResult &);
void categoryTotalHavingOneExpense(testcpplite::TestResult &);
} // namespace calculate
} // namespace budget
} // namespace sbash64

#endif
