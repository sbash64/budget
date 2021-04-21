#include "bank.hpp"
#include "constexpr-string.hpp"
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <numeric>

namespace sbash64::budget {
constexpr const std::array<char, 9> transferString{"transfer"};
constexpr auto transferFromMasterString{concatenate(
    transferString, std::array<char, 7>{" from "}, masterAccountName)};

static auto transferToString(std::string_view accountName) -> std::string {
  return concatenate(transferString, std::array<char, 5>{" to "}).data() +
         std::string{accountName};
}

static void credit(const std::shared_ptr<Account> &account,
                   const Transaction &t) {
  account->credit(t);
}

static void debit(const std::shared_ptr<Account> &account,
                  const Transaction &t) {
  account->debit(t);
}

static void verifyCredit(const std::shared_ptr<Account> &account,
                         const Transaction &t) {
  account->verifyCredit(t);
}

static void verifyDebit(const std::shared_ptr<Account> &account,
                        const Transaction &t) {
  account->verifyDebit(t);
}

static void removeCredit(const std::shared_ptr<Account> &account,
                         const Transaction &t) {
  account->removeCredit(t);
}

static void removeDebit(const std::shared_ptr<Account> &account,
                        const Transaction &t) {
  account->removeDebit(t);
}

static void
callIfObserverExists(Bank::Observer *observer,
                     const std::function<void(Bank::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
}

static void notifyThatTotalBalanceHasChanged(
    Bank::Observer *observer, const std::shared_ptr<Account> &primaryAccount,
    const std::map<std::string, std::shared_ptr<Account>, std::less<>>
        &secondaryAccounts) {
  callIfObserverExists(observer, [&](Bank::Observer *observer_) {
    observer_->notifyThatTotalBalanceHasChanged(std::accumulate(
        secondaryAccounts.begin(), secondaryAccounts.end(),
        primaryAccount->balance(),
        [](USD total, const std::pair<std::string_view,
                                      std::shared_ptr<Account>> &secondary) {
          const auto &[name, account] = secondary;
          return total + account->balance();
        }));
  });
}

static auto make(Account::Factory &factory, std::string_view name,
                 Model::Observer *observer) -> std::shared_ptr<Account> {
  auto account{factory.make(name)};
  callIfObserverExists(observer, [&](Bank::Observer *observer_) {
    observer_->notifyThatNewAccountHasBeenCreated(*account, name);
  });
  return account;
}

static auto
contains(std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
         std::string_view accountName) -> bool {
  return accounts.count(accountName) != 0;
}

static void createNewAccountIfNeeded(
    std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
    Account::Factory &factory, std::string_view accountName,
    Model::Observer *observer) {
  if (!contains(accounts, accountName))
    accounts[std::string{accountName}] = make(factory, accountName, observer);
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

static auto
at(const std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
   std::string_view name) -> const std::shared_ptr<Account> & {
  return accounts.at(std::string{name});
}

static auto makeAndLoad(Account::Factory &factory,
                        AccountDeserialization &deserialization,
                        std::string_view name, Model::Observer *observer)
    -> std::shared_ptr<Account> {
  auto account{make(factory, name, observer)};
  account->load(deserialization);
  return account;
}

Bank::Bank(Account::Factory &factory)
    : factory{factory}, primaryAccount{make(factory, masterAccountName.data(),
                                            observer)} {}

void Bank::attach(Observer *a) { observer = a; }

void Bank::credit(const Transaction &t) {
  budget::credit(primaryAccount, t);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void Bank::removeCredit(const Transaction &t) {
  budget::removeCredit(primaryAccount, t);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void Bank::debit(std::string_view accountName, const Transaction &t) {
  createNewAccountIfNeeded(secondaryAccounts, factory, accountName, observer);
  budget::debit(at(secondaryAccounts, accountName), t);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void Bank::removeDebit(std::string_view accountName, const Transaction &t) {
  if (contains(secondaryAccounts, accountName)) {
    budget::removeDebit(at(secondaryAccounts, accountName), t);
    notifyThatTotalBalanceHasChanged(observer, primaryAccount,
                                     secondaryAccounts);
  }
}

void Bank::transferTo(std::string_view accountName, USD amount, Date date) {
  createNewAccountIfNeeded(secondaryAccounts, factory, accountName, observer);
  budget::debit(primaryAccount, {amount, transferToString(accountName), date});
  budget::verifyDebit(primaryAccount,
                      {amount, transferToString(accountName), date});
  budget::credit(at(secondaryAccounts, accountName),
                 {amount, transferFromMasterString.data(), date});
  budget::verifyCredit(at(secondaryAccounts, accountName),
                       {amount, transferFromMasterString.data(), date});
}

void Bank::removeTransfer(std::string_view accountName, USD amount, Date date) {
  budget::removeDebit(primaryAccount,
                      {amount, transferToString(accountName), date});
  budget::removeCredit(at(secondaryAccounts, accountName),
                       {amount, transferFromMasterString.data(), date});
}

void Bank::show(View &view) {
  view.show(*primaryAccount, collect(secondaryAccounts));
}

void Bank::save(SessionSerialization &persistentMemory) {
  persistentMemory.save(*primaryAccount, collect(secondaryAccounts));
}

void Bank::load(SessionDeserialization &persistentMemory) {
  persistentMemory.load(*this);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void Bank::renameAccount(std::string_view from, std::string_view to) {
  at(secondaryAccounts, from)->rename(to);
}

auto Bank::findUnverifiedDebits(std::string_view accountName, USD amount)
    -> Transactions {
  return at(secondaryAccounts, accountName)->findUnverifiedDebits(amount);
}

auto Bank::findUnverifiedCredits(USD amount) -> Transactions {
  return primaryAccount->findUnverifiedCredits(amount);
}

void Bank::verifyDebit(std::string_view accountName, const Transaction &t) {
  budget::verifyDebit(at(secondaryAccounts, accountName), t);
}

void Bank::verifyCredit(const Transaction &t) {
  budget::verifyCredit(primaryAccount, t);
}

void Bank::removeAccount(std::string_view name) {
  if (contains(secondaryAccounts, name))
    secondaryAccounts.erase(std::string{name});
}

void Bank::notifyThatPrimaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  primaryAccount = makeAndLoad(factory, deserialization, name, observer);
}

void Bank::notifyThatSecondaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  secondaryAccounts[std::string{name}] =
      makeAndLoad(factory, deserialization, name, observer);
}

void Bank::reduce(const Date &date) {
  primaryAccount->reduce(date);
  for (const auto &[name, account] : secondaryAccounts)
    account->reduce(date);
}

static void callIfObserverExists(
    InMemoryAccount::Observer *observer,
    const std::function<void(InMemoryAccount::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
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

static void add(VerifiableTransactions &transactions,
                const VerifiableTransaction &t, Account::Observer *observer,
                const std::function<void(Account::Observer *,
                                         const Transaction &t)> &notify,
                const VerifiableTransactions &credits,
                const VerifiableTransactions &debits) {
  transactions.push_back(t);
  callIfObserverExists(observer, [&](InMemoryAccount::Observer *observer_) {
    notify(observer_, t.transaction);
    observer_->notifyThatBalanceHasChanged(balance(credits, debits));
  });
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

static void executeIfFound(
    VerifiableTransactions &transactions,
    const std::function<void(VerifiableTransactions::iterator)> &f,
    const std::function<bool(const VerifiableTransaction &)> &predicate) {
  const auto it{find_if(transactions.begin(), transactions.end(), predicate)};
  if (it != transactions.end())
    f(it);
}

static void
executeIfFound(VerifiableTransactions &transactions, const Transaction &t,
               const std::function<void(VerifiableTransactions::iterator)> &f) {
  return executeIfFound(transactions, f,
                        [&](const VerifiableTransaction &candidate) {
                          return candidate.transaction == t;
                        });
}

static void executeIfUnverifiedFound(
    VerifiableTransactions &transactions, const Transaction &t,
    const std::function<void(VerifiableTransactions::iterator)> &f) {
  return executeIfFound(
      transactions, f, [&](const VerifiableTransaction &candidate) {
        return candidate.transaction == t && !candidate.verified;
      });
}

static void verify(VerifiableTransactions &transactions, const Transaction &t) {
  executeIfUnverifiedFound(
      transactions, t,
      [&](VerifiableTransactions::iterator it) { it->verified = true; });
}

static void remove(VerifiableTransactions &transactions, const Transaction &t,
                   Account::Observer *observer,
                   const std::function<void(Account::Observer *,
                                            const Transaction &t)> &notify,
                   const VerifiableTransactions &credits,
                   const VerifiableTransactions &debits) {
  executeIfFound(transactions, t, [&](VerifiableTransactions::iterator it) {
    transactions.erase(it);
    callIfObserverExists(observer, [&](InMemoryAccount::Observer *observer_) {
      notify(observer_, t);
      observer_->notifyThatBalanceHasChanged(balance(credits, debits));
    });
  });
}

static auto findUnverified(const VerifiableTransactions &verifiableTransactions,
                           USD amount) -> Transactions {
  Transactions transactions;
  for (const auto &verifiableTransaction : verifiableTransactions)
    if (verifiableTransaction.transaction.amount == amount &&
        !verifiableTransaction.verified)
      transactions.push_back(verifiableTransaction.transaction);
  sort(transactions.begin(), transactions.end(),
       [](const Transaction &a, const Transaction &b) {
         return a.date < b.date;
       });
  return transactions;
}

InMemoryAccount::InMemoryAccount(std::string name) : name{std::move(name)} {}

void InMemoryAccount::attach(Observer *a) { observer = a; }

void InMemoryAccount::credit(const Transaction &t) {
  add(
      credits, {t, false}, observer,
      [](Observer *observer_, const Transaction &t_) {
        observer_->notifyThatCreditHasBeenAdded(t_);
      },
      credits, debits);
}

void InMemoryAccount::debit(const Transaction &t) {
  add(
      debits, {t, false}, observer,
      [](Observer *observer_, const Transaction &t_) {
        observer_->notifyThatDebitHasBeenAdded(t_);
      },
      credits, debits);
}

void InMemoryAccount::notifyThatCreditHasBeenDeserialized(
    const VerifiableTransaction &t) {
  add(
      credits, t, observer,
      [](Observer *observer_, const Transaction &t_) {
        observer_->notifyThatCreditHasBeenAdded(t_);
      },
      credits, debits);
}

void InMemoryAccount::notifyThatDebitHasBeenDeserialized(
    const VerifiableTransaction &t) {
  add(
      debits, t, observer,
      [](Observer *observer_, const Transaction &t_) {
        observer_->notifyThatDebitHasBeenAdded(t_);
      },
      credits, debits);
}

void InMemoryAccount::show(View &view) {
  view.showAccountSummary(name, budget::balance(credits, debits),
                          dateSortedTransactionsWithType(credits, debits));
}

void InMemoryAccount::save(AccountSerialization &serialization) {
  serialization.save(name, credits, debits);
}

void InMemoryAccount::load(AccountDeserialization &deserialization) {
  deserialization.load(*this);
}

void InMemoryAccount::removeDebit(const Transaction &t) {
  remove(
      debits, t, observer,
      [](Observer *observer_, const Transaction &t_) {
        observer_->notifyThatDebitHasBeenRemoved(t_);
      },
      credits, debits);
}

void InMemoryAccount::removeCredit(const Transaction &t) {
  remove(
      credits, t, observer,
      [](Observer *observer_, const Transaction &t_) {
        observer_->notifyThatCreditHasBeenRemoved(t_);
      },
      credits, debits);
}

void InMemoryAccount::rename(std::string_view s) { name = s; }

void InMemoryAccount::verifyCredit(const Transaction &t) { verify(credits, t); }

void InMemoryAccount::verifyDebit(const Transaction &t) { verify(debits, t); }

auto InMemoryAccount::findUnverifiedDebits(USD amount) -> Transactions {
  return findUnverified(debits, amount);
}

auto InMemoryAccount::findUnverifiedCredits(USD amount) -> Transactions {
  return findUnverified(credits, amount);
}

void InMemoryAccount::reduce(const Date &date) {
  VerifiableTransaction reduction{
      {budget::balance(credits, debits), "reduction", date}, true};
  callIfObserverExists(observer, [&](InMemoryAccount::Observer *observer_) {
    for (const auto &debit : debits)
      observer_->notifyThatDebitHasBeenRemoved(debit.transaction);
    for (const auto &credit : credits)
      observer_->notifyThatCreditHasBeenRemoved(credit.transaction);
    observer_->notifyThatCreditHasBeenAdded(reduction.transaction);
  });
  debits.clear();
  credits.clear();
  credits.push_back(reduction);
}

auto InMemoryAccount::balance() -> USD {
  return budget::balance(credits, debits);
}

auto InMemoryAccount::Factory::make(std::string_view name_)
    -> std::shared_ptr<Account> {
  return std::make_shared<InMemoryAccount>(std::string{name_});
}
} // namespace sbash64::budget
