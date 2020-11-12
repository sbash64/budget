#ifndef SBASH64_BUDGET_TEST_CALCULATE_HPP_
#define SBASH64_BUDGET_TEST_CALCULATE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::calculate {
void surplusHavingNoIncomeNorExpenses(testcpplite::TestResult &);
void surplusHavingNoExpenses(testcpplite::TestResult &);
void surplusHavingOneExpense(testcpplite::TestResult &);
void surplusHavingTwoExpenses(testcpplite::TestResult &);
void surplusHavingMultipleExpenses(testcpplite::TestResult &);
void surplusAfterExpenseChange(testcpplite::TestResult &);
void categoryTotalHavingNoExpenses(testcpplite::TestResult &);
void categoryTotalHavingOneUnrelatedExpense(testcpplite::TestResult &);
void categoryTotalHavingOneExpense(testcpplite::TestResult &);
void categoriesFromNoExpenses(testcpplite::TestResult &);
void categoriesFromOneExpense(testcpplite::TestResult &);
void categoriesFromTwoExpensesOfSameCategory(testcpplite::TestResult &);
void categoryTotalHavingMultipleExpenseTrees(testcpplite::TestResult &);
void categoryTotalHavingMultipleExpenseTrees2(testcpplite::TestResult &);
void categoryTotalHavingMultipleExpenseTrees3(testcpplite::TestResult &);
void categoryTotalHavingMultipleExpenseTrees4(testcpplite::TestResult &);
void categoryTotalHavingMultipleExpenseTrees5(testcpplite::TestResult &);
void categoryTotalHavingMultipleExpenseTrees6(testcpplite::TestResult &);
void totalHavingMultipleExpenseTrees(testcpplite::TestResult &);
} // namespace sbash64::budget::calculate

#endif
