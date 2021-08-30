#ifndef SBASH64_BUDGET_TEST_BUDGET_HPP_
#define SBASH64_BUDGET_TEST_BUDGET_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
void addsIncomeToIncomeAccount(testcpplite::TestResult &);
void addsExpenseToExpenseAccount(testcpplite::TestResult &);
void addsExpenseToExistingAccount(testcpplite::TestResult &);
void transfersFromIncomeToExpenseAccount(testcpplite::TestResult &);
void savesAccounts(testcpplite::TestResult &);
void loadsAccounts(testcpplite::TestResult &);
void clearsOldAccounts(testcpplite::TestResult &);
void removesExpenseFromAccount(testcpplite::TestResult &);
void doesNotRemoveExpenseFromNonexistentAccount(testcpplite::TestResult &);
void removesIncomeFromAccount(testcpplite::TestResult &);
void renamesAccount(testcpplite::TestResult &);
void removesAccount(testcpplite::TestResult &);
void findsUnverifiedDebitsFromAccount(testcpplite::TestResult &);
void findsUnverifiedCreditsFromMasterAccount(testcpplite::TestResult &);
void verifiesDebitForExistingAccount(testcpplite::TestResult &);
void verifiesCreditForMasterAccount(testcpplite::TestResult &);
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
void transfersAmountNeededToReachAllocation(testcpplite::TestResult &);
void transfersAmountFromAccountAllocatedSufficiently(testcpplite::TestResult &);
void restoresAccountsHavingNegativeBalances(testcpplite::TestResult &);
} // namespace sbash64::budget

#endif
