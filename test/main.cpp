#include "account.hpp"
#include "budget.hpp"
#include "format.hpp"
#include "parse.hpp"
#include "stream.hpp"
#include "transaction.hpp"

#include <sbash64/testcpplite/testcpplite.hpp>

#include <iostream>

namespace sbash64::budget {
static auto runAllTests() -> int {
  return testcpplite::test(
      {{parses::zeroDollars, "parses \"0\" as $0"},
       {parses::oneDollar, "parses \"1\" as $1"},
       {parses::oneDollarTwentyThreeCents, "parses \"1.23\" as $1.23"},
       {parses::oneDollarTwentyCentsWithoutTrailingZero,
        "parses \"1.2\" as $1.20"},
       {parses::oneCent, "parses \"0.01\" as 1¢"},
       {parses::tenCents, "parses \"0.10\" as 10¢"},
       {parses::twelveCentsWithoutLeadingZero, "parses \".12\" as 12¢"},
       {parses::twelveCentsIgnoringThirdDecimalPlace,
        "parses \"0.12X\" as 12¢"},
       {parses::oneCentIgnoringThirdDecimalPlace, "parses \"0.01X\" as 1¢"},
       {formats::zeroDollars, "formats 0¢ as \"$0.00\""},
       {formats::oneDollar, "formats $1 as \"$1.00\""},
       {formats::oneCent, "formats 1¢ as \"$0.01\""},
       {formats::tenCents, "formats 10¢ as \"$0.10\""},
       {formats::negativeOneDollarThirtyFourCents,
        "formats minus $1.34 as \"$-1.34\""},
       {formats::negativeFifteenCents, "formats minus 15¢ as \"$-0.15\""},
       {streams::fromBudget, "streams from budget"},
       {streams::toBudget, "streams to budget"},
       {streams::fromAccount, "streams from account"},
       {streams::nonfinalToAccount, "streams nonfinal to account"},
       {streams::finalToAccount, "streams final to account"},
       {streams::toAccountWithFunds, "streams to account with funds"},
       {streams::fromTransaction, "streams from transaction"},
       {streams::fromVerifiedTransaction, "streams from verified transaction"},
       {streams::toTransaction, "streams to transaction"},
       {streams::toVerifiedTransaction, "streams to verified transaction"},
       {createsAccount, "creates account"},
       {addsIncomeToIncomeAccount, "credits master account when credited"},
       {verifiesIncome, "verifies credit for master account"},
       {addsExpenseToExpenseAccount,
        "creates account when debiting nonexistent"},
       {addsExpenseToExistingAccount, "debits existing account"},
       {removesExpenseFromAccount, "removes transactions from accounts"},
       {verifiesExpenseForExistingAccount,
        "verifies debit for existing account"},
       {doesNotRemoveExpenseFromNonexistentAccount,
        "does nothing when removing debit from nonexistent account"},
       {removesIncomeFromAccount,
        "removes from master account when removing credit"},
       {transfersFromIncomeToExpenseAccount,
        "transfer debits master and credits other"},
       {savesAccounts, "save saves accounts"},
       {loadsAccounts, "load loads accounts"},
       {renamesAccount, "rename account"},
       {notifiesObserverOfDeserializedAccount,
        "notifies observer of new account"},
       {reducesEachAccount, "reduce reduces each account"},
       {notifiesThatNetIncomeHasChangedOnAddedIncome,
        "notifies that total balance has changed on credit"},
       {notifiesThatNetIncomeHasChangedOnAddExpense,
        "notifies that total balance has changed on debit"},
       {notifiesThatNetIncomeHasChangedOnRemoveAccount,
        "notifies that total balance has changed on remove account"},
       {removesAccount, "removes account"},
       {closesAccount, "closes account"},
       {closesAccountHavingNegativeBalance,
        "closes account having negative balance"},
       {clearsOldAccounts, "clearsOldAccounts"},
       {doesNotOverwriteExistingAccount, "doesNotOverwriteExistingAccount"},
       {transfersAmountNeededToReachAllocation,
        "transfersAmountNeededToReachAllocation"},
       {transfersAmountFromAccountAllocatedSufficiently,
        "transfersAmountFromAccountAllocatedSufficiently"},
       {restoresAccountsHavingNegativeBalances,
        "restoresAccountsHavingNegativeBalances"},
       {account::income::
            notifiesObserverOfUpdatedBalanceAfterAddingTransactions,
        "notifiesObserverOfUpdatedBalanceAfterAddingTransactions"},
       {account::income::
            notifiesObserverOfUpdatedBalanceAfterRemovingTransactions,
        "notifiesObserverOfUpdatedBalanceAfterRemovingTransactions"},
       {account::income::savesAllTransactionsAndAccountName,
        "savesAllTransactionRecordsAndAccountName"},
       {account::income::savesRemainingTransactionsAfterRemovingSome,
        "savesRemainingTransactionRecordsAfterRemovingSome"},
       {account::income::initializesAddedTransactions,
        "initializesTransactionRecords"},
       {account::income::hasTransactionsObserveDeserialization,
        "passesNewTransactionRecordsToDeserialization"},
       {account::income::savesLoadedTransactions,
        "savesTransactionRecordsLoaded"},
       {account::income::savesDuplicateTransactions,
        "savesDuplicateTransactionRecords"},
       {account::income::savesRemainingTransactionAfterRemovingVerified,
        "savesRemainingTransactionRecordsAfterRemovingVerified"},
       {account::income::notifiesObserverThatDuplicateTransactionsAreVerified,
        "notifiesDuplicateTransactionsAreVerified"},
       {account::income::notifiesObserverOfNewCredit,
        "notifiesObserverOfNewCredit"},
       {account::expense::notifiesObserverOfNewDebit,
        "notifiesObserverOfNewDebit"},
       {account::income::notifiesObserverOfVerifiedCredit,
        "notifiesCreditIsVerified"},
       {account::income::notifiesObserverOfRemovedCredit,
        "notifiesObserverOfRemovedCredit"},
       {account::income::reducesTransactionsToFunds,
        "reduceReducesToOneTransaction"},
       {account::income::removesTransactionsWhenReducing,
        "notifiesObserverOfTransactionsWhenReducing"},
       {account::income::returnsBalance, "returnsBalance"},
       {account::income::clearsReducedTransactions,
        "account reducesToNoTransactionsForZeroBalance"},
       {account::income::attemptsToRemoveEachCreditUntilFound,
        "attemptsToRemoveEachCreditUntilFound"},
       {account::income::notifiesObserverOfRemoval,
        "account::income::notifiesObserverOfRemoval"},
       {account::income::observesDeserialization,
        "account::income::observesDeserialization"},
       {account::expense::notifiesObserverOfRemoval,
        "account::expense::notifiesObserverOfRemoval"},
       {account::income::notifiesObserverOfUpdatedFundsAndBalanceOnClear,
        "account notifiesObserverOfUpdatedFundsAndBalanceOnClear"},
       {account::income::
            notifiesObserverOfUpdatedFundsAndBalanceOnSerialization,
        "account::income::notifiesObserverOfUpdatedFundsOnSerialization"},
       {account::expense::
            notifiesObserverOfUpdatedFundsAndBalanceOnSerialization,
        "account::expense::notifiesObserverOfUpdatedFundsOnSerialization"},
       {account::expense::initializesAddedTransactions,
        "account::expense::initializesAddedTransactions"},
       {transaction::notifiesObserverOfRemoval, "notifiesObserverWhenRemoved"},
       {transaction::savesLoadedTransaction, "savesWhatWasLoaded"},
       {transaction::observesDeserialization,
        "loadPassesSelfToDeserialization"},
       {transaction::notifiesObserverOfLoadedTransaction,
        "notifiesThatIsAfterReady"},
       {transaction::notifiesObserverOfInitializedTransaction,
        "notifiesThatIsAfterInitialize"},
       {transaction::removesLoadedTransaction, "removesLoadedValue"},
       {transaction::notifiesObserverOfRemovalByQuery,
        "notifiesObserverOfRemovalByQuery"},
       {transaction::doesNotRemoveUnequalTransaction,
        "doesNotRemoveUnequalValue"},
       {transaction::verifiesMatchingInitializedTransaction,
        "transaction verifies"},
       {transaction::doesNotVerifyMatchingInitializedTransactionTwice,
        "transaction doesNotVerifyTwice"},
       {transaction::notifiesObserverOfVerificationByQuery,
        "transaction notifiesObserverOfVerificationByQuery"},
       {transaction::savesVerificationByQuery,
        "transaction savesVerificationByQuery"},
       {transaction::notifiesObserverOfLoadedVerification,
        "transaction notifiesObserverOfLoadedVerification"},
       {transaction::savesInitializedTransaction,
        "transaction savesInitializedTransaction"},
       {transaction::removesInitializedTransaction,
        "transaction removesInitializedTransaction"},
       {transaction::doesNotVerifyUnequalInitializedTransaction,
        "transaction doesNotVerifyUnequalInitializedTransaction"}},
      std::cout);
}
} // namespace sbash64::budget

auto main() -> int { return sbash64::budget::runAllTests(); }
