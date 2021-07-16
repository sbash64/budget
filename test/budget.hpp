#ifndef SBASH64_BUDGET_TEST_BUDGET_HPP_
#define SBASH64_BUDGET_TEST_BUDGET_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
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
void removesAccount(testcpplite::TestResult &);
void findsUnverifiedDebitsFromAccount(testcpplite::TestResult &);
void findsUnverifiedCreditsFromMasterAccount(testcpplite::TestResult &);
void verifiesDebitForExistingAccount(testcpplite::TestResult &);
void verifiesCreditForMasterAccount(testcpplite::TestResult &);
void transferVerifiesTransactionsByDefault(testcpplite::TestResult &);
void notifiesObserverOfNewAccount(testcpplite::TestResult &);
void notifiesObserverOfRemovedAccount(testcpplite::TestResult &);
void reduceReducesEachAccount(testcpplite::TestResult &);
void notifiesThatTotalBalanceHasChangedOnCredit(testcpplite::TestResult &);
void notifiesThatTotalBalanceHasChangedOnRemoveAccount(
    testcpplite::TestResult &);
void createsAccount(testcpplite::TestResult &);
void closesAccount(testcpplite::TestResult &);
void closesAccountHavingNegativeBalance(testcpplite::TestResult &);
} // namespace sbash64::budget

#endif
