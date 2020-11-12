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
void expenseCategoryTotalHavingNoExpenses(testcpplite::TestResult &);
void expenseCategoryTotalHavingOneUnrelatedExpense(testcpplite::TestResult &);
void expenseCategoryTotalHavingOneExpense(testcpplite::TestResult &);
void categoriesFromNoExpenses(testcpplite::TestResult &);
void categoriesFromOneExpense(testcpplite::TestResult &);
void categoriesFromTwoExpensesOfSameCategory(testcpplite::TestResult &);
void expenseCategoryTotalHavingMultipleExpenseTrees(testcpplite::TestResult &);
void expenseCategoryTotalHavingMultipleExpenseTrees2(testcpplite::TestResult &);
void expenseCategoryTotalHavingMultipleExpenseTrees3(testcpplite::TestResult &);
void expenseCategoryTotalHavingMultipleExpenseTrees4(testcpplite::TestResult &);
void expenseCategoryTotalHavingMultipleExpenseTrees5(testcpplite::TestResult &);
void expenseCategoryTotalHavingMultipleExpenseTrees6(testcpplite::TestResult &);
void totalHavingMultipleExpenseTrees(testcpplite::TestResult &);
} // namespace sbash64::budget::calculate

#endif
