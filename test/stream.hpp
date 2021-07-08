#ifndef SBASH64_BUDGET_TEST_STREAM_HPP_
#define SBASH64_BUDGET_TEST_STREAM_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::stream {
void toTransactionRecord(testcpplite::TestResult &);
void fromAccounts(testcpplite::TestResult &);
void fromSession(testcpplite::TestResult &);
void toAccounts(testcpplite::TestResult &);
void toSession(testcpplite::TestResult &);
} // namespace sbash64::budget::stream

#endif
