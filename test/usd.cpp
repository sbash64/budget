#include "usd.hpp"
#include <gsl/gsl>

namespace sbash64 {
namespace budget {
auto operator"" _cents(unsigned long long cents) -> USD {
  return USD{gsl::narrow<int_least64_t>(cents)};
}
void assertEqual(testcpplite::TestResult &result, USD expected, USD actual) {
  assertEqual(result, gsl::narrow<unsigned long long>(expected.cents),
              gsl::narrow<unsigned long long>(actual.cents));
}
} // namespace budget
} // namespace sbash64
