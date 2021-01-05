#ifndef SBASH64_BUDGET_TEST_PRINT_HPP_
#define SBASH64_BUDGET_TEST_PRINT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::print {
void prettyBudgetHavingNoIncomeNorExpenses(testcpplite::TestResult &);
void formatZeroDollars(testcpplite::TestResult &);
void formatOneDollar(testcpplite::TestResult &);
void formatOneCent(testcpplite::TestResult &);
void formatTenCents(testcpplite::TestResult &);
void prettyBudgetHavingNoExpenses(testcpplite::TestResult &);
void prettyBudgetHavingOneExpense(testcpplite::TestResult &);
void prettyBudgetHavingMultipleExpenses(testcpplite::TestResult &);
void prettyBudgetHavingMultipleExpenseTrees(testcpplite::TestResult &);
void aFewExpenses(testcpplite::TestResult &);
void transactions(testcpplite::TestResult &);
void accounts(testcpplite::TestResult &);
void account(testcpplite::TestResult &);
} // namespace sbash64::budget::print

#endif
