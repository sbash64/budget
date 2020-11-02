#ifndef SBASH64_BUDGET_TEST_USD_HPP_
#define SBASH64_BUDGET_TEST_USD_HPP_

#include <gsl/gsl>
#include <sbash64/budget/budget.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64 {
namespace budget {
constexpr static auto operator"" _cents(unsigned long long cents) -> USD {
  return USD{gsl::narrow<int_least64_t>(cents)};
}

static void assertEqual(testcpplite::TestResult &result, USD expected,
                        USD actual) {
  assertEqual(result, gsl::narrow<unsigned long long>(expected.cents),
              gsl::narrow<unsigned long long>(actual.cents));
}
} // namespace budget
} // namespace sbash64

#endif
