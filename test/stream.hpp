#ifndef SBASH64_BUDGET_TEST_STREAM_HPP_
#define SBASH64_BUDGET_TEST_STREAM_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::stream {
void savesAccounts(testcpplite::TestResult &);
void savesSession(testcpplite::TestResult &);
void loadsAccounts(testcpplite::TestResult &);
void loadsSession(testcpplite::TestResult &);
} // namespace sbash64::budget::stream

#endif
