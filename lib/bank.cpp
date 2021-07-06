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
    observer_->notifyThatTotalBalanceHasChanged(accumulate(
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
                 TransactionRecord::Factory &transactionRecordFactory,
                 Model::Observer *observer) -> std::shared_ptr<Account> {
  auto account{factory.make(name, transactionRecordFactory)};
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
    TransactionRecord::Factory &transactionRecordFactory,
    Model::Observer *observer) {
  if (!contains(accounts, accountName))
    accounts[std::string{accountName}] =
        make(factory, accountName, transactionRecordFactory, observer);
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
                        std::string_view name,
                        TransactionRecord::Factory &transactionRecordFactory,
                        Model::Observer *observer) -> std::shared_ptr<Account> {
  auto account{make(factory, name, transactionRecordFactory, observer)};
  account->load(deserialization);
  return account;
}

Bank::Bank(Account::Factory &factory,
           TransactionRecord::Factory &transactionRecordFactory)
    : factory{factory}, transactionRecordFactory{transactionRecordFactory},
      primaryAccount{make(factory, masterAccountName.data(),
                          transactionRecordFactory, observer)} {}

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
  createNewAccountIfNeeded(secondaryAccounts, factory, accountName,
                           transactionRecordFactory, observer);
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
  createNewAccountIfNeeded(secondaryAccounts, factory, accountName,
                           transactionRecordFactory, observer);
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
  if (contains(secondaryAccounts, name)) {
    secondaryAccounts.erase(std::string{name});
    callIfObserverExists(observer, [&](Observer *observer_) {
      observer_->notifyThatAccountHasBeenRemoved(name);
    });
    notifyThatTotalBalanceHasChanged(observer, primaryAccount,
                                     secondaryAccounts);
  }
}

void Bank::notifyThatPrimaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  primaryAccount = makeAndLoad(factory, deserialization, name,
                               transactionRecordFactory, observer);
}

void Bank::notifyThatSecondaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  secondaryAccounts[std::string{name}] = makeAndLoad(
      factory, deserialization, name, transactionRecordFactory, observer);
}

void Bank::reduce(const Date &date) {
  primaryAccount->reduce(date);
  for (const auto &[name, account] : secondaryAccounts)
    account->reduce(date);
}
} // namespace sbash64::budget
