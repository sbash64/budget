#include "account.hpp"
#include "budget.hpp"
#include "format.hpp"
#include "parse.hpp"
#include "stream.hpp"

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
       {streams::fromTransaction, "streams from transaction"},
       {streams::fromVerifiedTransaction, "streams from verified transaction"},
       {streams::toTransaction, "streams to transaction"},
       {streams::toVerifiedTransaction, "streams to verified transaction"},
       {createsMasterAccountOnConstruction,
        "creates master account on construction"},
       {createsAccount, "creates account"},
       {creditsMasterAccountWhenCredited,
        "credits master account when credited"},
       {verifiesCreditForMasterAccount, "verifies credit for master account"},
       {createsAccountWhenDebitingNonexistent,
        "creates account when debiting nonexistent"},
       {debitsExistingAccount, "debits existing account"},
       {removesDebitFromAccount, "removes transactions from accounts"},
       {verifiesDebitForExistingAccount, "verifies debit for existing account"},
       {doesNothingWhenRemovingDebitFromNonexistentAccount,
        "does nothing when removing debit from nonexistent account"},
       {removesFromMasterAccountWhenRemovingCredit,
        "removes from master account when removing credit"},
       {transferDebitsMasterAndCreditsOther,
        "transfer debits master and credits other"},
       {transferVerifiesTransactionsByDefault,
        "transfer verifies transactions by default"},
       {removeTransferRemovesDebitFromMasterAndCreditFromOther,
        "remove transfer removes debit from master and credit from "
        "other"},
       {saveSavesAccounts, "save saves accounts"},
       {loadLoadsAccounts, "load loads accounts"},
       {renameAccount, "rename account"},
       {notifiesObserverOfNewAccount, "notifies observer of new account"},
       {notifiesObserverOfRemovedAccount,
        "notifies observer of removed account"},
       {reduceReducesEachAccount, "reduce reduces each account"},
       {notifiesThatTotalBalanceHasChangedOnCredit,
        "notifies that total balance has changed on credit"},
       {notifiesThatTotalBalanceHasChangedOnRemoveAccount,
        "notifies that total balance has changed on remove account"},
       {removesAccount, "removes account"},
       {closesAccount, "closes account"},
       {closesAccountHavingNegativeBalance,
        "closes account having negative balance"},
       {account::notifiesObserverOfUpdatedBalanceAfterAddingTransactions,
        "notifiesObserverOfUpdatedBalanceAfterAddingTransactions"},
       {account::notifiesObserverOfUpdatedBalanceAfterRemovingTransactions,
        "notifiesObserverOfUpdatedBalanceAfterRemovingTransactions"},
       {account::savesAllTransactionRecordsAndAccountName,
        "savesAllTransactionRecordsAndAccountName"},
       {account::savesRemainingTransactionRecordsAfterRemovingSome,
        "savesRemainingTransactionRecordsAfterRemovingSome"},
       {account::initializesTransactionRecords,
        "initializesTransactionRecords"},
       {account::passesSelfToDeserializationOnLoad,
        "passesSelfToDeserializationOnLoad"},
       {account::passesNewTransactionRecordsToDeserialization,
        "passesNewTransactionRecordsToDeserialization"},
       {account::savesTransactionRecordsLoaded,
        "savesTransactionRecordsLoaded"},
       {account::rename, "rename"},
       {account::savesDuplicateTransactionRecords,
        "savesDuplicateTransactionRecords"},
       {account::savesRemainingTransactionRecordsAfterRemovingVerified,
        "savesRemainingTransactionRecordsAfterRemovingVerified"},
       {account::notifiesDuplicateTransactionsAreVerified,
        "notifiesDuplicateTransactionsAreVerified"},
       {account::notifiesObserverOfNewCredit, "notifiesObserverOfNewCredit"},
       {account::notifiesObserverOfNewDebit, "notifiesObserverOfNewDebit"},
       {account::notifiesCreditIsVerified, "notifiesCreditIsVerified"},
       {account::notifiesDebitIsVerified, "notifiesDebitIsVerified"},
       {account::notifiesObserverOfRemovedDebit,
        "notifiesObserverOfRemovedDebit"},
       {account::notifiesObserverOfRemovedCredit,
        "notifiesObserverOfRemovedCredit"},
       {account::reduceReducesToOneTransaction,
        "reduceReducesToOneTransaction"},
       {account::notifiesObserverOfTransactionsWhenReducing,
        "notifiesObserverOfTransactionsWhenReducing"},
       {account::returnsBalance, "returnsBalance"},
       {account::reduceReducesToOneDebitForNegativeBalance,
        "reduceReducesToOneDebitForNegativeBalance"},
       {account::notifiesObserverWhenVerified, "notifiesObserverWhenVerified"},
       {account::saveAfterVerify, "saveAfterVerify"},
       {account::notifiesObserverWhenRemoved, "notifiesObserverWhenRemoved"},
       {account::savesWhatWasLoaded, "savesWhatWasLoaded"},
       {account::loadPassesSelfToDeserialization,
        "loadPassesSelfToDeserialization"},
       {account::notifiesThatIsAfterReady, "notifiesThatIsAfterReady"},
       {account::notifiesThatIsAfterInitialize,
        "notifiesThatIsAfterInitialize"}},
      std::cout);
}
} // namespace sbash64::budget

auto main() -> int { return sbash64::budget::runAllTests(); }
