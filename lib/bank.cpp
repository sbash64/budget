#include "bank.hpp"

namespace sbash64::budget {
Bank::Bank(AccountFactory &factory)
    : factory{factory}, masterAccount{factory.make("master")} {}

void Bank::credit(const Transaction &t) { masterAccount->credit(t); }

static void createNewAccountIfNeeded(
    std::map<std::string, std::shared_ptr<Account>> &accounts,
    AccountFactory &factory, std::string_view accountName) {
  if (accounts.count(accountName.data()) == 0)
    accounts[accountName.data()] = factory.make(accountName);
}

void Bank::debit(std::string_view accountName, const Transaction &t) {
  createNewAccountIfNeeded(accounts, factory, accountName);
  const auto account{accounts.at(accountName.data())};
  account->debit(t);
}

void Bank::transferTo(std::string_view accountName, USD amount, Date date) {
  createNewAccountIfNeeded(accounts, factory, accountName);
  masterAccount->debit(
      Transaction{amount, "transfer to " + std::string{accountName}, date});
  accounts.at(accountName.data())
      ->credit(Transaction{amount, "transfer from master", date});
}
} // namespace sbash64::budget