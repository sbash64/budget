#include "bank.hpp"
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <numeric>

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
constexpr auto transferFromMasterString{
    concatenate(transferDescription, " from ", masterAccountName)};
constexpr auto transferToString{concatenate(transferDescription, " to ")};

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
  budget::debit(
      masterAccount,
      Transaction{amount, transferToString.c + std::string{accountName}, date});
  budget::credit(accounts.at(std::string{accountName}),
                 Transaction{amount, transferFromMasterString.c, date});
}

void Bank::removeTransfer(std::string_view accountName, USD amount, Date date) {
  budget::removeDebit(
      masterAccount,
      Transaction{amount, transferToString.c + std::string{accountName}, date});
  budget::removeCredit(accounts.at(std::string{accountName}),
                       Transaction{amount, transferFromMasterString.c, date});
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

void Bank::renameAccount(std::string_view from, std::string_view to) {
  accounts.at(std::string{from})->rename(to);
}

InMemoryAccount::InMemoryAccount(std::string name) : name{std::move(name)} {}

void InMemoryAccount::credit(const Transaction &t) {
  credits.push_back({t, false});
}

void InMemoryAccount::debit(const Transaction &t) {
  debits.push_back({t, false});
}

static auto balance(const VerifiableTransactions &transactions) -> USD {
  return accumulate(transactions.begin(), transactions.end(), USD{0},
                    [](USD total, const VerifiableTransaction &t) {
                      return total + t.transaction.amount;
                    });
}

static auto balance(const VerifiableTransactions &credits,
                    const VerifiableTransactions &debits) -> USD {
  return balance(credits) - balance(debits);
}

static auto
dateSortedTransactionsWithType(const VerifiableTransactions &credits,
                               const VerifiableTransactions &debits)
    -> std::vector<VerifiableTransactionWithType> {
  std::vector<VerifiableTransactionWithType> transactions;
  transactions.reserve(credits.size() + debits.size());
  for (const auto &c : credits)
    transactions.push_back({c, Transaction::Type::credit});
  for (const auto &d : debits)
    transactions.push_back({d, Transaction::Type::debit});
  sort(transactions.begin(), transactions.end(),
       [](const VerifiableTransactionWithType &a,
          const VerifiableTransactionWithType &b) {
         return a.verifiableTransaction.transaction.date <
                b.verifiableTransaction.transaction.date;
       });
  return transactions;
}

void InMemoryAccount::show(View &view) {
  view.showAccountSummary(name, balance(credits, debits),
                          dateSortedTransactionsWithType(credits, debits));
}

void InMemoryAccount::save(SessionSerialization &serialization) {
  serialization.saveAccount(name, credits, debits);
}

auto InMemoryAccount::Factory::make(std::string_view name)
    -> std::shared_ptr<Account> {
  return std::make_shared<InMemoryAccount>(std::string{name});
}

void InMemoryAccount::load(SessionDeserialization &deserialization) {
  deserialization.loadAccount(credits, debits);
}

static void remove(VerifiableTransactions &transactions, const Transaction &t) {
  const auto it{find(transactions.begin(), transactions.end(),
                     VerifiableTransaction{t, false})};
  if (it != transactions.end())
    transactions.erase(it);
}

void InMemoryAccount::removeDebit(const Transaction &t) { remove(debits, t); }

void InMemoryAccount::removeCredit(const Transaction &t) { remove(credits, t); }

void InMemoryAccount::rename(std::string_view s) { name = s; }

void InMemoryAccount::verifyCredit(const Transaction &) {}

void InMemoryAccount::verifyDebit(const Transaction &) {}
} // namespace sbash64::budget