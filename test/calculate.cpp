#include "calculate.hpp"
#include "usd.hpp"
#include <sbash64/budget/calculate.hpp>

namespace sbash64 {
namespace budget {
namespace calculate {
void differenceHavingNoIncomeNorExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, difference(Income{0_cents}, Expenses{}));
}
} // namespace calculate
} // namespace budget
} // namespace sbash64
