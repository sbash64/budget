#include "account.hpp"
#include "bank.hpp"
#include "parse.hpp"
#include "print.hpp"
#include "stream.hpp"
#include <iostream>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
static auto runAllTests() -> int {
  return testcpplite::test(
      {{parse::zeroAsUsd, "parse zero (0) as USD"},
       {parse::oneAsUsd, "parse one (1) as USD"},
       {parse::numberHavingTwoDecimalPlacesAsUsd,
        "parse number having two decimal places (X.XX) as USD"},
       {parse::numberHavingOneDecimalPlaceAsUsd,
        "parse number having one decimal place (X.X) as USD"},
       {parse::oneOneHundredthAsUsd, "parse one one-hundredth (0.01) as USD"},
       {parse::oneTenthAsUsd, "parse one-tenth (0.1) as USD"},
       {parse::numberWithoutLeadingZeroAsUsd,
        "parse number without leading zero (.X) as USD"},
       {parse::numberHavingThreeDecimalPlacesAsUsd,
        "parse number having three decimal places (X.XXX) as USD"},
       {parse::twelveOneThousandthsAsUsd,
        "parse twelve one-thousandths (0.012) as USD"},
       {format::zeroDollars, "format zero dollars"},
       {format::oneDollar, "format one dollar"},
       {format::oneCent, "format one cent"},
       {format::tenCents, "format ten cents"},
       {format::negativeOneDollarThirtyFourCents,
        "format negative one dollar and thirty-four cents"},
       {format::negativeFifteenCents, "format negative fifteen cents"},
       {stream::toTransactionRecord, "stream loads transaction record"},
       {stream::fromAccounts, "stream saves accounts"},
       {stream::toAccounts, "stream loads accounts"},
       {stream::fromSession, "stream saves session"},
       {stream::toSession, "stream loads session"},
       {bank::createsMasterAccountOnConstruction,
        "bank creates master account on construction"},
       {bank::creditsMasterAccountWhenCredited,
        "bank credits master account when credited"},
       {bank::verifiesCreditForMasterAccount,
        "bank verifies credit for master account"},
       {bank::createsAccountWhenDebitingNonexistent,
        "bank creates account when debiting nonexistent"},
       {bank::debitsExistingAccount, "bank debits existing account"},
       {bank::removesDebitFromAccount,
        "bank removes transactions from accounts"},
       {bank::verifiesDebitForExistingAccount,
        "bank verifies debit for existing account"},
       {bank::doesNothingWhenRemovingDebitFromNonexistentAccount,
        "bank does nothing when removing debit from nonexistent account"},
       {bank::removesFromMasterAccountWhenRemovingCredit,
        "bank removes from master account when removing credit"},
       {bank::transferDebitsMasterAndCreditsOther,
        "bank transfer debits master and credits other"},
       {bank::transferVerifiesTransactionsByDefault,
        "bank transfer verifies transactions by default"},
       {bank::removeTransferRemovesDebitFromMasterAndCreditFromOther,
        "bank remove transfer removes debit from master and credit from "
        "other"},
       {bank::showShowsAccountsInAlphabeticOrder, "bank show shows accounts"},
       {bank::saveSavesAccounts, "bank save saves accounts"},
       {bank::loadLoadsAccounts, "bank load loads accounts"},
       {bank::renameAccount, "bank rename account"},
       {bank::notifiesObserverOfNewAccount,
        "bank notifies observer of new account"},
       {bank::notifiesObserverOfRemovedAccount,
        "bank notifies observer of removed account"},
       {bank::reduceReducesEachAccount, "bank reduce reduces each account"},
       {bank::notifiesThatTotalBalanceHasChangedOnCredit,
        "bank notifies that total balance has changed on credit"},
       {bank::notifiesThatTotalBalanceHasChangedOnRemoveAccount,
        "bank notifies that total balance has changed on remove account"},
       {bank::removesAccount, "bank removes account"},
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
       {account::notifiesObserverWhenRemoved, "notifiesObserverWhenRemoved"}},
      std::cout);
}
} // namespace sbash64::budget

auto main() -> int { return sbash64::budget::runAllTests(); }
