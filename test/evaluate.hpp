#ifndef SBASH64_BUDGET_TEST_EVALUATE_HPP_
#define SBASH64_BUDGET_TEST_EVALUATE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::evaluate {
void expenseWithOneSubcategory(testcpplite::TestResult &);
void expenseWithTwoSubcategories(testcpplite::TestResult &);
void expenseWithMultiWordSubcategories(testcpplite::TestResult &);
void invalidExpense(testcpplite::TestResult &);
void printCommand(testcpplite::TestResult &);
void expenseShouldPrintExpense(testcpplite::TestResult &);
} // namespace sbash64::budget::evaluate

#endif
