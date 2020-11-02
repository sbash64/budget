#include "parse.hpp"
#include <sbash64/budget/parse.hpp>

namespace sbash64 {
namespace budget {
namespace parse {
constexpr static auto operator"" _cents(unsigned long long cents) -> USD {
  return USD{static_cast<int_least64_t>(cents)};
}

static void assertEqual(testcpplite::TestResult &result, USD expected,
                        USD actual) {
  assertEqual(result, static_cast<unsigned long long>(expected.cents),
              static_cast<unsigned long long>(actual.cents));
}

void zero(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, usd("0"));
}

void one(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents, usd("1"));
}
} // namespace parse
} // namespace budget
} // namespace sbash64
