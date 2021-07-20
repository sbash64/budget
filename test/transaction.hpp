#ifndef SBASH64_BUDGET_TEST_TRANSACTION_HPP_
#define SBASH64_BUDGET_TEST_TRANSACTION_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::transaction {
void verifies(testcpplite::TestResult &);
void doesNotVerifyTwice(testcpplite::TestResult &);
void notifiesObserverOfVerification(testcpplite::TestResult &);
void notifiesObserverOfVerificationByQuery(testcpplite::TestResult &);
void savesVerification(testcpplite::TestResult &);
void notifiesObserverOfRemoval(testcpplite::TestResult &);
void savesLoadedValue(testcpplite::TestResult &);
void loadPassesSelfToDeserialization(testcpplite::TestResult &);
void notifiesObserverOfLoadedValue(testcpplite::TestResult &);
void notifiesObserverOfInitializedValue(testcpplite::TestResult &);
void notifiesObserverOfRemovalByQuery(testcpplite::TestResult &);
void removesLoadedValue(testcpplite::TestResult &);
void doesNotRemoveUnequalValue(testcpplite::TestResult &);
} // namespace sbash64::budget::transaction

#endif
