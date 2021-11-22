#ifndef SBASH64_BUDGET_TEST_PARSE_HPP_
#define SBASH64_BUDGET_TEST_PARSE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::parses {
void zeroDollars(testcpplite::TestResult &);
void oneDollar(testcpplite::TestResult &);
void oneDollarTwentyThreeCents(testcpplite::TestResult &);
void oneDollarTwentyCentsWithoutTrailingZero(testcpplite::TestResult &);
void oneCent(testcpplite::TestResult &);
void tenCents(testcpplite::TestResult &);
void twelveCentsWithoutLeadingZero(testcpplite::TestResult &);
void twelveCentsIgnoringThirdDecimalPlace(testcpplite::TestResult &);
void oneCentIgnoringThirdDecimalPlace(testcpplite::TestResult &);
void unknownValuesToZero(testcpplite::TestResult &);
} // namespace sbash64::budget::parses

#endif
