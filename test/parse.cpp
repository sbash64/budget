#include "parse.hpp"
#include "usd.hpp"
#include <sbash64/budget/parse.hpp>

namespace sbash64::budget::parse {
static void assertEqual(testcpplite::TestResult &result, USD expected,
                        std::string_view s) {
  assertEqual(result, expected, usd(s));
}

void zeroAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, "0");
}

void oneAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 100_cents, "1");
}

void twoDecimalPlacesAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 123_cents, "1.23");
}

void oneDecimalPlaceAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 120_cents, "1.2");
}

void oneOneHundredthAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents, "0.01");
}

void oneTenthAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 10_cents, "0.10");
}

void withoutLeadingZeroAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 12_cents, ".12");
}

void threeDecimalPlacesAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 12_cents, ".126");
}

void twelveOneThousandthsAsUsd(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents, ".012");
}

static void assertIsNotUsd(testcpplite::TestResult &result,
                           std::string_view s) {
  assertFalse(result, isUsd(s));
}

static void assertIsUsd(testcpplite::TestResult &result, std::string_view s) {
  assertTrue(result, isUsd(s));
}

void alphabeticIsNotUsd(testcpplite::TestResult &result) {
  assertIsNotUsd(result, "a");
}

void integerIsUsd(testcpplite::TestResult &result) { assertIsUsd(result, "1"); }

void decimalIsUsd(testcpplite::TestResult &result) {
  assertIsUsd(result, "1.2");
}
} // namespace sbash64::budget::parse
