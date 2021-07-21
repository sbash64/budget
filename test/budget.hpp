#ifndef SBASH64_BUDGET_TEST_BUDGET_HPP_
#define SBASH64_BUDGET_TEST_BUDGET_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
void createsMasterAccount(testcpplite::TestResult &);
void creditsMasterAccount(testcpplite::TestResult &);
void debitsNonexistentAccount(testcpplite::TestResult &);
void debitsExistingAccount(testcpplite::TestResult &);
void transfersFromMasterAccountToOther(testcpplite::TestResult &);
void savesAccounts(testcpplite::TestResult &);
void loadsAccounts(testcpplite::TestResult &);
void clearsOldAccounts(testcpplite::TestResult &);
void removesDebit(testcpplite::TestResult &);
void doesNotRemoveDebitFromNonexistentAccount(testcpplite::TestResult &);
void removesCredit(testcpplite::TestResult &);
void removesTransfer(testcpplite::TestResult &);
void renamesAccount(testcpplite::TestResult &);
void removesAccount(testcpplite::TestResult &);
void findsUnverifiedDebitsFromAccount(testcpplite::TestResult &);
void findsUnverifiedCreditsFromMasterAccount(testcpplite::TestResult &);
void verifiesDebitForExistingAccount(testcpplite::TestResult &);
void verifiesCreditForMasterAccount(testcpplite::TestResult &);
void verifiesTransferTransactions(testcpplite::TestResult &);
void notifiesObserverOfDeserializedAccount(testcpplite::TestResult &);
void reducesEachAccount(testcpplite::TestResult &);
void notifiesThatTotalBalanceHasChangedOnCredit(testcpplite::TestResult &);
void notifiesThatTotalBalanceHasChangedOnDebit(testcpplite::TestResult &);
void notifiesThatTotalBalanceHasChangedOnRemoveAccount(
    testcpplite::TestResult &);
void createsAccount(testcpplite::TestResult &);
void doesNotOverwriteExistingAccount(testcpplite::TestResult &);
void closesAccount(testcpplite::TestResult &);
void closesAccountHavingNegativeBalance(testcpplite::TestResult &);
} // namespace sbash64::budget

#endif
