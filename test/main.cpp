#include "account.hpp"
#include "bank.hpp"
#include "evaluate.hpp"
#include "file.hpp"
#include "parse.hpp"
#include "print.hpp"
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
       {evaluate::print, "evaluate print command"},
       {evaluate::transferTo, "evaluate transfer to command"},
       {evaluate::debit, "evaluate debit"},
       {evaluate::credit, "evaluate credit"},
       {bank::createsMasterAccountOnConstruction,
        "bank creates master account on construction"},
       {bank::creditsMasterAccountWhenCredited,
        "bank credits master account when credited"},
       {bank::debitsNonexistantAccount, "bank debits nonexistant account"},
       {bank::debitsExistingAccount, "bank debits existing account"},
       {bank::transferDebitsMasterAndCreditsOther,
        "bank transfer debits master and credits other"},
       {bank::showShowsAccountsInAlphabeticOrder, "bank show shows accounts"},
       {bank::saveSavesAccounts, "bank save saves accounts"},
       {bank::loadLoadsAccounts, "bank load loads accounts"},
       {account::showShowsAllTransactionsInChronologicalOrderAndBalance,
        "account show shows all transactions and balance"},
       {account::saveSavesAllTransactions,
        "account save saves all transactions"},
       {account::loadLoadsAllTransactions,
        "account load loads all transactions"},
       {print::formatZeroDollars, "format zero dollars"},
       {print::formatOneDollar, "format one dollar"},
       {print::formatOneCent, "format one cent"},
       {print::formatTenCents, "format ten cents"},
       {print::accounts, "print accounts"},
       {print::account, "print account"},
       {file::savesAccounts, "file saves accounts"},
       {file::savesAccount, "file saves account"},
       {file::loadsAccounts, "file loads accounts"},
       {file::loadsAccount, "file loads account"}},
      std::cout);
}
} // namespace sbash64::budget

int main() { return sbash64::budget::runAllTests(); }
