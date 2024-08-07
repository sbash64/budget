#ifndef SBASH64_BUDGET_TEST_TRANSACTION_HPP_
#define SBASH64_BUDGET_TEST_TRANSACTION_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::transaction {
void verifiesMatchingInitializedTransaction(testcpplite::TestResult &);
void doesNotVerifyUnequalInitializedTransaction(testcpplite::TestResult &);
void doesNotVerifyMatchingInitializedTransactionTwice(
    testcpplite::TestResult &);
void notifiesObserverOfVerificationByQuery(testcpplite::TestResult &);
void savesVerificationByQuery(testcpplite::TestResult &);
void notifiesObserverOfRemoval(testcpplite::TestResult &);
void savesInitializedTransaction(testcpplite::TestResult &);
void savesArchival(testcpplite::TestResult &);
void notifiesObserverOfInitializedTransaction(testcpplite::TestResult &);
void notifiesObserverOfRemovalByQuery(testcpplite::TestResult &);
void notifiesObserverOfArchival(testcpplite::TestResult &);
void doesNotNotifyObserverOfArchivalTwice(testcpplite::TestResult &);
void removesInitializedTransaction(testcpplite::TestResult &);
void doesNotRemoveUnequalTransaction(testcpplite::TestResult &);
} // namespace sbash64::budget::transaction

#endif
