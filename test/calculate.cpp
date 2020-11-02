#include "calculate.hpp"
#include <gsl/gsl>
#include <sbash64/budget/calculate.hpp>

namespace sbash64 {
namespace budget {
namespace calculate {
constexpr static auto operator"" _cents(unsigned long long cents) -> USD {
  return USD{gsl::narrow<int_least64_t>(cents)};
}

static void assertEqual(testcpplite::TestResult &result, USD expected,
                        USD actual) {
  assertEqual(result, gsl::narrow<unsigned long long>(expected.cents),
              gsl::narrow<unsigned long long>(actual.cents));
}

void differenceHavingNoIncomeNorExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, difference(Income{0_cents}, Expenses{}));
}
} // namespace calculate
} // namespace budget
} // namespace sbash64
