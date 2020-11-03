#include "calculate.hpp"
#include "usd.hpp"
#include <sbash64/budget/calculate.hpp>

namespace sbash64 {
namespace budget {
namespace calculate {
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
} // namespace calculate
} // namespace budget
} // namespace sbash64
