#ifndef SBASH64_BUDGET_TEST_BANK_HPP_
#define SBASH64_BUDGET_TEST_BANK_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::bank {
void createsMasterAccountOnConstruction(testcpplite::TestResult &);
void creditsMasterAccountWhenCredited(testcpplite::TestResult &);
void createsAccountWhenDebitingNonexistent(testcpplite::TestResult &);
void debitsExistingAccount(testcpplite::TestResult &);
void transferDebitsMasterAndCreditsOther(testcpplite::TestResult &);
void showShowsAccountsInAlphabeticOrder(testcpplite::TestResult &);
void saveSavesAccounts(testcpplite::TestResult &);
void loadLoadsAccounts(testcpplite::TestResult &);
void removesDebitFromAccount(testcpplite::TestResult &);
void doesNothingWhenRemovingDebitFromNonexistentAccount(
    testcpplite::TestResult &);
void removesFromMasterAccountWhenRemovingCredit(testcpplite::TestResult &);
void removeTransferRemovesDebitFromMasterAndCreditFromOther(
    testcpplite::TestResult &);
void renameAccount(testcpplite::TestResult &);
void findsUnverifiedDebitsFromAccount(testcpplite::TestResult &);
void findsUnverifiedCreditsFromMasterAccount(testcpplite::TestResult &);
void verifiesDebitForExistingAccount(testcpplite::TestResult &);
void verifiesCreditForMasterAccount(testcpplite::TestResult &);
void transferVerifiesTransactionsByDefault(testcpplite::TestResult &);
void notifiesObserverOfNewDebitWhenDebited(testcpplite::TestResult &);
void notifiesObserverOfNewCreditWhenCredited(testcpplite::TestResult &);
void notifiesObserverOfNewCreditAndDebitWhenTransferred(
    testcpplite::TestResult &);
void notifiesObserverOfNewAccount(testcpplite::TestResult &);
} // namespace sbash64::budget::bank

#endif
