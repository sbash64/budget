#include "parse.hpp"
#include <gsl/gsl>
#include <sbash64/budget/parse.hpp>

namespace sbash64 {
namespace budget {
namespace parse {
constexpr static auto operator"" _cents(unsigned long long cents) -> USD {
  return USD{gsl::narrow<int_least64_t>(cents)};
}

static void assertEqual(testcpplite::TestResult &result, USD expected,
                        USD actual) {
  assertEqual(result, gsl::narrow<unsigned long long>(expected.cents),
              gsl::narrow<unsigned long long>(actual.cents));
}

static void assertEqual(testcpplite::TestResult &result, USD expected,
                        std::string_view s) {
  assertEqual(result, expected, usd(s));
}

void zero(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, "0");
}

void one(testcpplite::TestResult &result) {
  assertEqual(result, 100_cents, "1");
}

void twoDecimalPlaces(testcpplite::TestResult &result) {
  assertEqual(result, 123_cents, "1.23");
}

void oneDecimalPlace(testcpplite::TestResult &result) {
  assertEqual(result, 120_cents, "1.2");
}

void oneOneHundredth(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents, "0.01");
}

void oneTenth(testcpplite::TestResult &result) {
  assertEqual(result, 10_cents, "0.10");
}

void withoutLeadingZero(testcpplite::TestResult &result) {
  assertEqual(result, 12_cents, ".12");
}

void threeDecimalPlaces(testcpplite::TestResult &result) {
  assertEqual(result, 12_cents, ".126");
}
} // namespace parse
} // namespace budget
} // namespace sbash64
