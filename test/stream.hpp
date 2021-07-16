#ifndef SBASH64_BUDGET_TEST_STREAM_HPP_
#define SBASH64_BUDGET_TEST_STREAM_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::stream {
void toTransaction(testcpplite::TestResult &);
void toVerifiedTransaction(testcpplite::TestResult &);
void fromAccount(testcpplite::TestResult &);
void fromBudget(testcpplite::TestResult &);
void toAccounts(testcpplite::TestResult &);
void toAccounts2(testcpplite::TestResult &);
void toSession(testcpplite::TestResult &);
} // namespace sbash64::budget::stream

#endif
