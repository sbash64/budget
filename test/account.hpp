#ifndef SBASH64_BUDGET_TEST_ACCOUNT_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &);
void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &);
void savesAllTransactionRecordsAndAccountName(testcpplite::TestResult &);
void savesRemainingTransactionRecordsAfterRemovingSome(
    testcpplite::TestResult &);
void initializesTransactionRecords(testcpplite::TestResult &);
void passesSelfToDeserializationOnLoad(testcpplite::TestResult &);
void passesNewTransactionRecordsToDeserialization(testcpplite::TestResult &);
void savesTransactionRecordsLoaded(testcpplite::TestResult &);
void rename(testcpplite::TestResult &);
void savesRemainingTransactionRecordsAfterRemovingVerified(
    testcpplite::TestResult &);
void notifiesDuplicateTransactionsAreVerified(testcpplite::TestResult &);
void savesDuplicateTransactionRecords(testcpplite::TestResult &);
void notifiesObserverOfNewCredit(testcpplite::TestResult &);
void notifiesObserverOfNewDebit(testcpplite::TestResult &);
void notifiesCreditIsVerified(testcpplite::TestResult &);
void notifiesDebitIsVerified(testcpplite::TestResult &);
void notifiesObserverOfRemovedDebit(testcpplite::TestResult &);
void notifiesObserverOfRemovedCredit(testcpplite::TestResult &);
void reduceReducesToOneTransaction(testcpplite::TestResult &);
void notifiesObserverOfTransactionsWhenReducing(testcpplite::TestResult &);
void returnsBalance(testcpplite::TestResult &);
void reduceReducesToOneDebitForNegativeBalance(testcpplite::TestResult &);
void notifiesObserverWhenVerified(testcpplite::TestResult &);
} // namespace sbash64::budget::account

#endif
