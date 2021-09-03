#ifndef SBASH64_BUDGET_TEST_ACCOUNT_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
void notifiesObserverOfRemoval(testcpplite::TestResult &);
void observesDeserialization(testcpplite::TestResult &);
void savesNewName(testcpplite::TestResult &);
void notifiesObserverOfUpdatedFundsOnDeposit(testcpplite::TestResult &);
void notifiesObserverOfUpdatedFundsOnWithdraw(testcpplite::TestResult &);
void initializesAddedTransactions(testcpplite::TestResult &);
void notifiesObserverOfNewCredit(testcpplite::TestResult &);
void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &);
void savesAllTransactionsAndAccountName(testcpplite::TestResult &);
void attemptsToRemoveEachCreditUntilFound(testcpplite::TestResult &);
void savesLoadedTransactions(testcpplite::TestResult &);
void savesRemainingTransactionsAfterRemovingSome(testcpplite::TestResult &);
void savesRemainingTransactionAfterRemovingVerified(testcpplite::TestResult &);
void savesDuplicateTransactions(testcpplite::TestResult &);
void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &);
void hasTransactionsObserveDeserialization(testcpplite::TestResult &);
void notifiesObserverThatDuplicateTransactionsAreVerified(
    testcpplite::TestResult &);
void notifiesObserverOfVerifiedCredit(testcpplite::TestResult &);
void notifiesObserverOfRemovedCredit(testcpplite::TestResult &);
void reducesTransactionsToFunds(testcpplite::TestResult &);
void clearsReducedTransactions(testcpplite::TestResult &);
void removesTransactionsWhenReducing(testcpplite::TestResult &);
void returnsBalance(testcpplite::TestResult &);
void notifiesObserverOfUpdatedFundsOnReduce(testcpplite::TestResult &);
void notifiesObserverOfUpdatedFundsAndBalanceOnClear(testcpplite::TestResult &);
void notifiesObserverOfUpdatedFundsAndBalanceOnSerialization(
    testcpplite::TestResult &);
void withdrawsFromFunds(testcpplite::TestResult &);
void depositsToFunds(testcpplite::TestResult &);
} // namespace sbash64::budget::account

#endif
