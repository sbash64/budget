#ifndef SBASH64_BUDGET_TEST_PARSE_HPP_
#define SBASH64_BUDGET_TEST_PARSE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::parse {
void zeroAsUsd(testcpplite::TestResult &);
void oneAsUsd(testcpplite::TestResult &);
void twoDecimalPlacesAsUsd(testcpplite::TestResult &);
void oneDecimalPlaceAsUsd(testcpplite::TestResult &);
void oneOneHundredthAsUsd(testcpplite::TestResult &);
void oneTenthAsUsd(testcpplite::TestResult &);
void withoutLeadingZeroAsUsd(testcpplite::TestResult &);
void threeDecimalPlacesAsUsd(testcpplite::TestResult &);
void twelveOneThousandthsAsUsd(testcpplite::TestResult &);
void alphabeticIsNotUsd(testcpplite::TestResult &);
void integerIsUsd(testcpplite::TestResult &);
} // namespace sbash64::budget::parse

#endif
