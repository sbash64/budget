#ifndef SBASH64_BUDGET_TEST_STREAM_HPP_
#define SBASH64_BUDGET_TEST_STREAM_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::streams {
void fromTransaction(testcpplite::TestResult &);
void fromVerifiedTransaction(testcpplite::TestResult &);
void toTransaction(testcpplite::TestResult &);
void toVerifiedTransaction(testcpplite::TestResult &);
void fromAccount(testcpplite::TestResult &);
void fromBudget(testcpplite::TestResult &);
void nonfinalToAccount(testcpplite::TestResult &);
void finalToAccount(testcpplite::TestResult &);
void toBudget(testcpplite::TestResult &);
} // namespace sbash64::budget::streams

#endif
