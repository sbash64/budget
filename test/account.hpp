#ifndef SBASH64_BUDGET_TEST_ACCOUNT_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &);
void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &);
void attemptsToRemoveEachDebitUntilFound(testcpplite::TestResult &);
void attemptsToRemoveEachCreditUntilFound(testcpplite::TestResult &);
void savesAllTransactionsAndAccountName(testcpplite::TestResult &);
void savesRemainingTransactionsAfterRemovingSome(testcpplite::TestResult &);
void initializesAddedTransactions(testcpplite::TestResult &);
void notifiesObserverOfRemoval(testcpplite::TestResult &);
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
void reducesToOneTransaction(testcpplite::TestResult &);
void removesTransactionsWhenReducing(testcpplite::TestResult &);
void returnsBalance(testcpplite::TestResult &);
void withdrawsFromFunds(testcpplite::TestResult &);
void depositsToFunds(testcpplite::TestResult &);
void reducesToOneDebitForNegativeBalance(testcpplite::TestResult &);
void reducesToOneCreditForPositiveBalance(testcpplite::TestResult &);
void reducesToNoTransactionsForZeroBalance(testcpplite::TestResult &);
} // namespace sbash64::budget::account

#endif
