#ifndef SBASH64_BUDGET_TEST_ACCOUNT_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
void showShowsAllTransactionsInChronologicalOrderAndBalance(
    testcpplite::TestResult &);
void showAfterRemoveShowsRemainingTransactions(testcpplite::TestResult &);
void showShowsVerifiedTransactionsInChronologicalOrderAndBalance(
    testcpplite::TestResult &);
void saveSavesAllTransactions(testcpplite::TestResult &);
void loadLoadsAllTransactions(testcpplite::TestResult &);
void rename(testcpplite::TestResult &);
void findUnverifiedDebitsReturnsUnverifiedDebitsMatchingAmount(
    testcpplite::TestResult &);
void findUnverifiedCreditsReturnsUnverifiedCreditsMatchingAmount(
    testcpplite::TestResult &);
} // namespace sbash64::budget::account

#endif
