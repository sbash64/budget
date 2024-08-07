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
void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &);
void notifiesObserverThatDuplicateTransactionsAreVerified(
    testcpplite::TestResult &);
void verifiesLoadedTransaction(testcpplite::TestResult &);
void archivesLoadedTransaction(testcpplite::TestResult &);
void notifiesObserverOfVerifiedTransaction(testcpplite::TestResult &);
void notifiesObserverOfRemovedTransaction(testcpplite::TestResult &);
void notifiesUpdatedBalanceAfterArchivingVerified(testcpplite::TestResult &);
void archivesVerifiedTransactions(testcpplite::TestResult &);
void returnsBalance(testcpplite::TestResult &);
void notifiesObserverOfUpdatedFundsOnReduce(testcpplite::TestResult &);
void notifiesObserverOfUpdatedBalanceOnClear(testcpplite::TestResult &);
void notifiesObserverOfUpdatedFundsAndBalanceOnSerialization(
    testcpplite::TestResult &);
void increasesAllocationByAmountArchived(testcpplite::TestResult &);
void decreasesAllocationByAmountArchived(testcpplite::TestResult &);
void notifiesObserverOfIncreasedAllocation(testcpplite::TestResult &);
void notifiesObserverOfDecreasedAllocation(testcpplite::TestResult &);
void notifiesObserverOfLoadedAllocation(testcpplite::TestResult &);
void doesNotRemoveArchivedTransaction(testcpplite::TestResult &);
} // namespace sbash64::budget::account

#endif
