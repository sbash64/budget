#include "parse.hpp"
#include "usd.hpp"

#include <sbash64/budget/parse.hpp>

namespace sbash64::budget::parses {
static void assertEqual(testcpplite::TestResult &result, USD expected,
                        std::string_view s) {
  assertEqual(result, expected, usd(s));
}

void zeroDollars(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, "0");
}

void oneDollar(testcpplite::TestResult &result) {
  assertEqual(result, 100_cents, "1");
}

void oneDollarTwentyThreeCents(testcpplite::TestResult &result) {
  assertEqual(result, 123_cents, "1.23");
}

void oneDollarTwentyCentsWithoutTrailingZero(testcpplite::TestResult &result) {
  assertEqual(result, 120_cents, "1.2");
}

void oneCent(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents, "0.01");
}

void tenCents(testcpplite::TestResult &result) {
  assertEqual(result, 10_cents, "0.10");
}

void twelveCentsWithoutLeadingZero(testcpplite::TestResult &result) {
  assertEqual(result, 12_cents, ".12");
}

void twelveCentsIgnoringThirdDecimalPlace(testcpplite::TestResult &result) {
  assertEqual(result, 12_cents, ".126");
}

void oneCentIgnoringThirdDecimalPlace(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents, ".012");
}

void unknownValuesAsZero(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, "?");
  assertEqual(result, 0_cents, "hello world");
  assertEqual(result, 0_cents, "-10.23");
  assertEqual(result, 0_cents, ".-23");
}
} // namespace sbash64::budget::parses
