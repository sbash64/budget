#include "format.hpp"
#include "usd.hpp"

#include <sbash64/budget/format.hpp>

#include <sstream>

namespace sbash64::budget {
static void assertFormatYields(testcpplite::TestResult &result, USD usd,
                               std::string_view expected) {
  std::stringstream stream;
  putWithDollarSign(stream, usd);
  assertEqual(result, std::string{expected}, stream.str());
}

namespace formats {
void zeroDollars(testcpplite::TestResult &result) {
  assertFormatYields(result, 0_cents, "$0.00");
}

void oneDollar(testcpplite::TestResult &result) {
  assertFormatYields(result, 100_cents, "$1.00");
}

void oneCent(testcpplite::TestResult &result) {
  assertFormatYields(result, 1_cents, "$0.01");
}

void tenCents(testcpplite::TestResult &result) {
  assertFormatYields(result, 10_cents, "$0.10");
}

void negativeOneDollarThirtyFourCents(testcpplite::TestResult &result) {
  assertFormatYields(result, -134_cents, "$-1.34");
}

void negativeFifteenCents(testcpplite::TestResult &result) {
  assertFormatYields(result, -15_cents, "$-0.15");
}
} // namespace formats
} // namespace sbash64::budget
