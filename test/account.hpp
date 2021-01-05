#ifndef SBASH64_BUDGET_TEST_ACCOUNT_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
void printPrintsAllTransactionsInChronologicalOrderAndBalance(
    testcpplite::TestResult &);
} // namespace sbash64::budget::account

#endif
