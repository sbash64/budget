#ifndef SBASH64_BUDGET_TEST_PARSE_HPP_
#define SBASH64_BUDGET_TEST_PARSE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::parse {
void zeroAsUsd(testcpplite::TestResult &);
void oneAsUsd(testcpplite::TestResult &);
void numberHavingTwoDecimalPlacesAsUsd(testcpplite::TestResult &);
void numberHavingOneDecimalPlaceAsUsd(testcpplite::TestResult &);
void oneOneHundredthAsUsd(testcpplite::TestResult &);
void oneTenthAsUsd(testcpplite::TestResult &);
void numberWithoutLeadingZeroAsUsd(testcpplite::TestResult &);
void numberHavingThreeDecimalPlacesAsUsd(testcpplite::TestResult &);
void twelveOneThousandthsAsUsd(testcpplite::TestResult &);
} // namespace sbash64::budget::parse

#endif
