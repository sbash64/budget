#ifndef SBASH64_BUDGET_TEST_PARSE_HPP_
#define SBASH64_BUDGET_TEST_PARSE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::parse {
void zero(testcpplite::TestResult &);
void one(testcpplite::TestResult &);
void twoDecimalPlaces(testcpplite::TestResult &);
void oneDecimalPlace(testcpplite::TestResult &);
void oneOneHundredth(testcpplite::TestResult &);
void oneTenth(testcpplite::TestResult &);
void withoutLeadingZero(testcpplite::TestResult &);
void threeDecimalPlaces(testcpplite::TestResult &);
void twelveOneThousandths(testcpplite::TestResult &);
} // namespace sbash64::budget::parse

#endif
