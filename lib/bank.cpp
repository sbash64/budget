#include "bank.hpp"

namespace sbash64::budget {
constexpr auto masterAccountName{"master"};
constexpr auto transferDescription{"transfer"};

static void credit(const std::shared_ptr<Account> &account,
                   const Transaction &t) {
  account->credit(t);
}

static void debit(const std::shared_ptr<Account> &account,
                  const Transaction &t) {
  account->debit(t);
}

Bank::Bank(AccountFactory &factory)
    : factory{factory}, masterAccount{factory.make(masterAccountName)} {}

void Bank::credit(const Transaction &t) { budget::credit(masterAccount, t); }

static void createNewAccountIfNeeded(
    std::map<std::string, std::shared_ptr<Account>> &accounts,
    AccountFactory &factory, std::string_view accountName) {
  if (accounts.count(accountName.data()) == 0)
    accounts[accountName.data()] = factory.make(accountName);
}

void Bank::debit(std::string_view accountName, const Transaction &t) {
  createNewAccountIfNeeded(accounts, factory, accountName);
  budget::debit(accounts.at(accountName.data()), t);
}

void Bank::transferTo(std::string_view accountName, USD amount, Date date) {
  createNewAccountIfNeeded(accounts, factory, accountName);
  budget::debit(masterAccount,
                Transaction{amount,
                            std::string{transferDescription} + " to " +
                                std::string{accountName},
                            date});
  budget::credit(accounts.at(accountName.data()),
                 Transaction{amount,
                             std::string{transferDescription} + " from " +
                                 std::string{masterAccountName},
                             date});
}

static auto
collect(const std::map<std::string, std::shared_ptr<Account>> &accounts)
    -> std::vector<const Account *> {
  std::vector<const Account *> collected;
  collected.reserve(accounts.size());
  for (const auto &[name, account] : accounts)
    collected.push_back(account.get());
  return collected;
}

void Bank::print(Printer &printer) {
  printer.print(*masterAccount, collect(accounts));
}
} // namespace sbash64::budget