#include <cstdint>
#include <string_view>

namespace sbash64 {
namespace budget {
struct USD {
  std::int_least64_t cents;
};
namespace parse {
auto usd(std::string_view) -> USD { return {}; }
} // namespace parse
} // namespace budget
} // namespace sbash64

#include <sbash64/testcpplite/testcpplite.hpp>
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
} // namespace parse
} // namespace budget
} // namespace sbash64

#include <iostream>
#include <sbash64/testcpplite/testcpplite.hpp>

int main() {
  sbash64::testcpplite::test({{sbash64::budget::parse::zero, "parse zero"}},
                             std::cout);
}
