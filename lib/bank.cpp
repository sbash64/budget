#include "bank.hpp"
#include <initializer_list>

namespace sbash64::budget {
// https://stackoverflow.com/a/65440575

// we cannot return a char array from a function, therefore we need a wrapper
template <unsigned N> struct String { char c[N]; };

template <unsigned... Len>
constexpr auto concatenate(const char (&...strings)[Len]) {
  constexpr auto N{(... + Len) - sizeof...(Len)};
  String<N + 1> result = {};
  result.c[N] = '\0';

  auto *dst{result.c};
  for (const auto *src : {strings...})
    for (; *src != '\0'; src++, dst++)
      *dst = *src;
  return result;
}

constexpr const char masterAccountName[]{"master"};
constexpr const char transferDescription[]{"transfer"};
constexpr auto transferFromMaster{
    concatenate(transferDescription, " from ", masterAccountName)};

static void credit(const std::shared_ptr<Account> &account,
                   const Transaction &t) {
  account->credit(t);
}

static void debit(const std::shared_ptr<Account> &account,
                  const Transaction &t) {
  account->debit(t);
}

static void removeCredit(const std::shared_ptr<Account> &account,
                         const Transaction &t) {
  account->removeCredit(t);
}

static void removeDebit(const std::shared_ptr<Account> &account,
                        const Transaction &t) {
  account->removeDebit(t);
}

Bank::Bank(Account::Factory &factory)
    : factory{factory}, masterAccount{factory.make(masterAccountName)} {}

void Bank::credit(const Transaction &t) { budget::credit(masterAccount, t); }

void Bank::removeCredit(const Transaction &t) {
  budget::removeCredit(masterAccount, t);
}

static auto
contains(std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
         std::string_view accountName) -> bool {
  return accounts.count(accountName) != 0;
}

static void createNewAccountIfNeeded(
    std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
    Account::Factory &factory, std::string_view accountName) {
  if (!contains(accounts, accountName))
    accounts[std::string{accountName}] = factory.make(accountName);
}

void Bank::debit(std::string_view accountName, const Transaction &t) {
  createNewAccountIfNeeded(accounts, factory, accountName);
  budget::debit(accounts.at(std::string{accountName}), t);
}

void Bank::removeDebit(std::string_view accountName, const Transaction &t) {
  if (contains(accounts, accountName))
    budget::removeDebit(accounts.at(std::string{accountName}), t);
}

void Bank::transferTo(std::string_view accountName, USD amount, Date date) {
  createNewAccountIfNeeded(accounts, factory, accountName);
  budget::debit(masterAccount,
                Transaction{amount,
                            concatenate(transferDescription, " to ").c +
                                std::string{accountName},
                            date});
  budget::credit(accounts.at(std::string{accountName}),
                 Transaction{amount, transferFromMaster.c, date});
}

void Bank::removeTransfer(std::string_view accountName, USD amount, Date date) {
  budget::removeDebit(masterAccount,
                      Transaction{amount,
                                  concatenate(transferDescription, " to ").c +
                                      std::string{accountName},
                                  date});
  budget::removeCredit(accounts.at(std::string{accountName}),
                       Transaction{amount, transferFromMaster.c, date});
}

static auto collect(const std::map<std::string, std::shared_ptr<Account>,
                                   std::less<>> &accounts)
    -> std::vector<Account *> {
  std::vector<Account *> collected;
  collected.reserve(accounts.size());
  for (const auto &[name, account] : accounts)
    collected.push_back(account.get());
  return collected;
}

void Bank::show(View &view) { view.show(*masterAccount, collect(accounts)); }

void Bank::save(SessionSerialization &persistentMemory) {
  persistentMemory.save(*masterAccount, collect(accounts));
}

void Bank::load(SessionDeserialization &persistentMemory) {
  persistentMemory.load(factory, masterAccount, accounts);
}
} // namespace sbash64::budget