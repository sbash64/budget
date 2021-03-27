#include "account.hpp"
#include "bank.hpp"
#include "command-line.hpp"
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
       {parse::twoDecimalPlacesAsUsd, "parse two decimal places (X.XX) as USD"},
       {parse::oneDecimalPlaceAsUsd, "parse one decimal place (X.X) as USD"},
       {parse::oneOneHundredthAsUsd, "parse one one-hundredth (0.01) as USD"},
       {parse::oneTenthAsUsd, "parse one-tenth (0.1) as USD"},
       {parse::withoutLeadingZeroAsUsd,
        "parse without leading zero (.X) as USD"},
       {parse::threeDecimalPlacesAsUsd,
        "parse three decimal places (X.XXX) as USD"},
       {parse::twelveOneThousandthsAsUsd,
        "parse twelve one-thousandths (0.012) as USD"},
       {command_line::print, "command line parses print command"},
       {command_line::transferTo, "command line parses transfer command"},
       {command_line::debit, "command line parses debit command"},
       {command_line::debitMultiWordAccount,
        "command line parses debit command for account with multiple words in "
        "its name"},
       {command_line::debitPromptsForDate,
        "command line prompts for date during debit command"},
       {command_line::debitPromptsForAccountName,
        "command line prompts for account name during debit command"},
       {command_line::debitPromptsForAmount,
        "command line prompts for amount during debit command"},
       {command_line::creditPromptsForAmount,
        "command line prompts for amount during credit command"},
       {command_line::debitPromptsForDesriptionAfterDateEntered,
        "command line prompts for description after date is entered during "
        "debit command"},
       {command_line::debitShowsTransaction,
        "command line shows debited transaction"},
       {command_line::credit, "command line parses credit command"},
       {command_line::save, "command line parses save command"},
       {command_line::load, "command line parses load command"},
       {command_line::renameAccount, "command line parses rename command"},
       {command_line::renameAccountPromptsForNewName,
        "command line prompts for new account name during rename command"},
       {command_line::removeDebit,
        "command line parses \"remove debit\" command"},
       {command_line::removeCredit,
        "command line parses \"remove credit\" command"},
       {command_line::removeTransfer,
        "command line parses \"remove transfer\" command"},
       {command_line::verifyDebit,
        "command line parses \"verify debit\" command"},
       {command_line::verifyCredit,
        "command line parses \"verify credit\" command"},
       {command_line::showsDebitCandidatesForVerification,
        "command line shows the debit candidates for verification"},
       {command_line::showsMessageWhenNoCandidatesFoundForVerification,
        "command line shows message when no candidates found for verification"},
       {command_line::
            showsDebitCandidatesForVerificationAgainWhenSelectedDebitIsOutOfRange,
        "command line shows the debit candidates for verification again when "
        "selected debit is out of range"},
       {command_line::promptsForDebitVerificationConfirmation,
        "command line prompts for debit verification confirmation"},
       {command_line::verifyOnlyDebitFound,
        "command line verifies only debit found"},
       {command_line::unrecognizedCommandPrintsMessage,
        "command line shows message on unrecognized command"},
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
       {bank::findsUnverifiedDebitsFromAccount,
        "bank finds unverified debits from account"},
       {bank::findsUnverifiedCreditsFromMasterAccount,
        "bank finds unverified credits from master account"},
       {bank::doesNothingWhenRemovingDebitFromNonexistentAccount,
        "bank does nothing when removing debit from nonexistent account"},
       {bank::removesFromMasterAccountWhenRemovingCredit,
        "bank removes from master account when removing credit"},
       {bank::transferDebitsMasterAndCreditsOther,
        "bank transfer debits master and credits other"},
       {bank::transferVerifiesTransactionsByDefault,
        "bank transfer verifies transactions by default"},
       {bank::removeTransferRemovesDebitFromMasterAndCreditFromOther,
        "bank remove transfer removes debit from master and credit from other"},
       {bank::showShowsAccountsInAlphabeticOrder, "bank show shows accounts"},
       {bank::saveSavesAccounts, "bank save saves accounts"},
       {bank::loadLoadsAccounts, "bank load loads accounts"},
       {bank::renameAccount, "bank rename account"},
       {bank::notifiesObserverOfNewDebitWhenDebited,
        "bank notifies observer of new debit when debited"},
       {bank::notifiesObserverOfNewCreditWhenCredited,
        "bank notifies observer of new credit when credited"},
       {account::showShowsAllTransactionsInChronologicalOrderAndBalance,
        "account show shows all transactions and balance"},
       {account::showAfterRemoveShowsRemainingTransactions,
        "account show after remove shows remaining transactions"},
       {account::showAfterRemovingVerifiedTransactionShowsRemaining,
        "account show after removing verified transaction shows remaining"},
       {account::showShowsVerifiedTransactions,
        "account show shows verified transactions"},
       {account::showShowsDuplicateVerifiedTransactions,
        "account show shows verified transactions having same amount"},
       {account::findUnverifiedDebitsReturnsUnverifiedDebitsMatchingAmount,
        "account find unverified debits returns unverified debits "
        "matching amount"},
       {account::findUnverifiedCreditsReturnsUnverifiedCreditsMatchingAmount,
        "account find unverified credits returns unverified credits "
        "matching amount"},
       {account::saveSavesAllTransactions,
        "account save saves all transactions"},
       {account::loadLoadsAllTransactions,
        "account load loads all transactions"},
       {account::rename, "account rename"},
       {print::formatZeroDollars, "format zero dollars"},
       {print::formatOneDollar, "format one dollar"},
       {print::formatOneCent, "format one cent"},
       {print::formatTenCents, "format ten cents"},
       {print::formatNegativeDollarThirtyFour, "format negative ten cents"},
       {print::accounts, "print accounts"},
       {print::account, "print account"},
       {print::prompt, "print prompt"},
       {print::transactionWithSuffix, "print transaction with suffix"},
       {print::enumeratedTransactions, "print enumerated transactions"},
       {print::message, "print message"},
       {file::savesAccounts, "file saves accounts"},
       {file::loadsAccounts, "file loads accounts"}},
      std::cout);
}
} // namespace sbash64::budget

int main() { return sbash64::budget::runAllTests(); }
