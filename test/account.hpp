#ifndef SBASH64_BUDGET_TEST_ACCOUNT_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &);
void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &);
void savesAllTransactionsAndAccountName(testcpplite::TestResult &);
void savesRemainingTransactionsAfterRemovingSome(testcpplite::TestResult &);
void initializesAddedTransactions(testcpplite::TestResult &);
void observesDeserialization(testcpplite::TestResult &);
void hasTransactionsObserveDeserialization(testcpplite::TestResult &);
void savesLoadedTransactions(testcpplite::TestResult &);
void savesNewName(testcpplite::TestResult &);
void savesRemainingTransactionAfterRemovingVerified(testcpplite::TestResult &);
void notifiesObserverThatDuplicateTransactionsAreVerified(
    testcpplite::TestResult &);
void savesDuplicateTransactions(testcpplite::TestResult &);
void notifiesObserverOfNewCredit(testcpplite::TestResult &);
void notifiesObserverOfNewDebit(testcpplite::TestResult &);
void notifiesObserverOfVerifiedCredit(testcpplite::TestResult &);
void notifiesObserverOfVerifiedDebit(testcpplite::TestResult &);
void notifiesObserverOfRemovedDebit(testcpplite::TestResult &);
void notifiesObserverOfRemovedCredit(testcpplite::TestResult &);
void reduceReducesToOneTransaction(testcpplite::TestResult &);
void notifiesObserverOfTransactionsWhenReducing(testcpplite::TestResult &);
void returnsBalance(testcpplite::TestResult &);
void reduceReducesToOneDebitForNegativeBalance(testcpplite::TestResult &);
void notifiesObserverWhenVerified(testcpplite::TestResult &);
void saveAfterVerify(testcpplite::TestResult &);
void notifiesObserverWhenRemoved(testcpplite::TestResult &);
void savesWhatWasLoaded(testcpplite::TestResult &);
void loadPassesSelfToDeserialization(testcpplite::TestResult &);
void notifiesThatIsAfterReady(testcpplite::TestResult &);
void notifiesThatIsAfterInitialize(testcpplite::TestResult &);
} // namespace sbash64::budget::account

#endif
