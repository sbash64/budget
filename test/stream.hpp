#ifndef SBASH64_BUDGET_TEST_STREAM_HPP_
#define SBASH64_BUDGET_TEST_STREAM_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::file {
void savesAccounts(testcpplite::TestResult &);
void savesSession(testcpplite::TestResult &);
void loadsAccounts(testcpplite::TestResult &);
} // namespace sbash64::budget::file

#endif
