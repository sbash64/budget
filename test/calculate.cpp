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
} // namespace calculate
} // namespace budget
} // namespace sbash64
