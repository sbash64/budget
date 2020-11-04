#include "calculate.hpp"
#include "usd.hpp"
#include <algorithm>
#include <sbash64/budget/calculate.hpp>

namespace sbash64::budget::calculate {
void differenceHavingNoIncomeNorExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, difference(Income{0_cents}, Expenses{}));
}

void differenceHavingIncomeButNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents, difference(Income{1_cents}, Expenses{}));
}

void differenceHavingOneExpense(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents,
              difference(Income{3_cents}, Expenses{{Expense{2_cents}}}));
}

void differenceHavingTwoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 6_cents,
              difference(Income{11_cents},
                         Expenses{{Expense{2_cents}, Expense{3_cents}}}));
}

void categoryTotalHavingNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, total(Category{"miscellaneous"}, Expenses{}));
}

void categoryTotalHavingOneUnrelatedExpense(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents,
              total(Category{"miscellaneous"},
                    Expenses{{Expense{1_cents, Category{"groceries"}}}}));
}

void categoryTotalHavingOneExpense(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents,
              total(Category{"miscellaneous"},
                    Expenses{{Expense{1_cents, Category{"miscellaneous"}}}}));
}

static void assertEqual(testcpplite::TestResult &result,
                        const Categories &expected, const Categories &actual) {
  assertEqual(result, expected.each.size(), actual.each.size());
  assertTrue(result, std::equal(expected.each.begin(), expected.each.end(),
                                actual.each.begin()));
}

void categoriesFromNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, Categories{}, categories(Expenses{}));
}

void categoriesFromOneExpense(testcpplite::TestResult &result) {
  assertEqual(result, Categories{{Category{"groceries"}}},
              categories(Expenses{{Expense{0_cents, Category{"groceries"}}}}));
}

void categoriesFromTwoExpensesOfSameCategory(testcpplite::TestResult &result) {
  assertEqual(result, Categories{{Category{"groceries"}}},
              categories(Expenses{{Expense{1_cents, Category{"groceries"}},
                                   Expense{2_cents, Category{"groceries"}}}}));
}
} // namespace sbash64::budget::calculate
