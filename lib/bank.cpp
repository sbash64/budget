#include "bank.hpp"
#include "constexpr-string.hpp"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <sstream>
#include <string_view>
#include <utility>

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
callIfObserverExists(BudgetInMemory::Observer *observer,
                     const std::function<void(BudgetInMemory::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
}

static void notifyThatTotalBalanceHasChanged(
    BudgetInMemory::Observer *observer,
    const std::shared_ptr<Account> &primaryAccount,
    const std::map<std::string, std::shared_ptr<Account>, std::less<>>
        &secondaryAccounts) {
  callIfObserverExists(observer, [&](BudgetInMemory::Observer *observer_) {
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
                 Budget::Observer *observer) -> std::shared_ptr<Account> {
  auto account{factory.make(name, transactionRecordFactory)};
  callIfObserverExists(observer, [&](BudgetInMemory::Observer *observer_) {
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
    Budget::Observer *observer) {
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
                        Budget::Observer *observer)
    -> std::shared_ptr<Account> {
  auto account{make(factory, name, transactionRecordFactory, observer)};
  account->load(deserialization);
  return account;
}

BudgetInMemory::BudgetInMemory(
    Account::Factory &factory,
    TransactionRecord::Factory &transactionRecordFactory)
    : factory{factory}, transactionRecordFactory{transactionRecordFactory},
      primaryAccount{make(factory, masterAccountName.data(),
                          transactionRecordFactory, observer)} {}

void BudgetInMemory::attach(Observer *a) { observer = a; }

void BudgetInMemory::credit(const Transaction &t) {
  budget::credit(primaryAccount, t);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void BudgetInMemory::removeCredit(const Transaction &t) {
  budget::removeCredit(primaryAccount, t);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void BudgetInMemory::debit(std::string_view accountName, const Transaction &t) {
  createNewAccountIfNeeded(secondaryAccounts, factory, accountName,
                           transactionRecordFactory, observer);
  budget::debit(at(secondaryAccounts, accountName), t);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void BudgetInMemory::removeDebit(std::string_view accountName,
                                 const Transaction &t) {
  if (contains(secondaryAccounts, accountName)) {
    budget::removeDebit(at(secondaryAccounts, accountName), t);
    notifyThatTotalBalanceHasChanged(observer, primaryAccount,
                                     secondaryAccounts);
  }
}

template <std::size_t N>
auto transaction(USD amount, std::array<char, N> description, Date date)
    -> Transaction {
  return {amount, description.data(), date};
}

static auto transferToTransaction(USD amount, std::string_view accountName,
                                  Date date) -> Transaction {
  return {amount, transferToString(accountName), date};
}

void BudgetInMemory::transferTo(std::string_view accountName, USD amount,
                                Date date) {
  createNewAccountIfNeeded(secondaryAccounts, factory, accountName,
                           transactionRecordFactory, observer);
  budget::debit(primaryAccount,
                transferToTransaction(amount, accountName, date));
  budget::verifyDebit(primaryAccount,
                      transferToTransaction(amount, accountName, date));
  budget::credit(at(secondaryAccounts, accountName),
                 transaction(amount, transferFromMasterString, date));
  budget::verifyCredit(at(secondaryAccounts, accountName),
                       transaction(amount, transferFromMasterString, date));
}

void BudgetInMemory::removeTransfer(std::string_view accountName, USD amount,
                                    Date date) {
  budget::removeDebit(primaryAccount,
                      {amount, transferToString(accountName), date});
  budget::removeCredit(at(secondaryAccounts, accountName),
                       {amount, transferFromMasterString.data(), date});
}

void BudgetInMemory::save(BudgetSerialization &persistentMemory) {
  persistentMemory.save(*primaryAccount, collect(secondaryAccounts));
}

void BudgetInMemory::load(BudgetDeserialization &persistentMemory) {
  persistentMemory.load(*this);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void BudgetInMemory::renameAccount(std::string_view from, std::string_view to) {
  at(secondaryAccounts, from)->rename(to);
}

void BudgetInMemory::verifyDebit(std::string_view accountName,
                                 const Transaction &t) {
  budget::verifyDebit(at(secondaryAccounts, accountName), t);
}

void BudgetInMemory::verifyCredit(const Transaction &t) {
  budget::verifyCredit(primaryAccount, t);
}

static void
remove(std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
       std::string_view name) {
  at(accounts, name)->remove();
  accounts.erase(std::string{name});
}

void BudgetInMemory::removeAccount(std::string_view name) {
  if (contains(secondaryAccounts, name)) {
    remove(secondaryAccounts, name);
    notifyThatTotalBalanceHasChanged(observer, primaryAccount,
                                     secondaryAccounts);
  }
}

void BudgetInMemory::notifyThatPrimaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  primaryAccount = makeAndLoad(factory, deserialization, name,
                               transactionRecordFactory, observer);
}

void BudgetInMemory::notifyThatSecondaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  secondaryAccounts[std::string{name}] = makeAndLoad(
      factory, deserialization, name, transactionRecordFactory, observer);
}

void BudgetInMemory::reduce(const Date &date) {
  primaryAccount->reduce(date);
  for (const auto &[name, account] : secondaryAccounts)
    account->reduce(date);
}

void BudgetInMemory::createAccount(std::string_view name) {
  createNewAccountIfNeeded(secondaryAccounts, factory, name,
                           transactionRecordFactory, observer);
}

static auto
transaction(const std::map<std::string, std::shared_ptr<Account>, std::less<>>
                &accounts,
            std::string_view name, const std::stringstream &description,
            const Date &date) -> Transaction {
  return {at(accounts, name)->balance(), description.str(), date};
}

void BudgetInMemory::closeAccount(std::string_view name, const Date &date) {
  if (contains(secondaryAccounts, name)) {
    std::stringstream description;
    description << "close " << name;
    budget::credit(primaryAccount,
                   transaction(secondaryAccounts, name, description, date));
    budget::verifyCredit(primaryAccount, transaction(secondaryAccounts, name,
                                                     description, date));
    remove(secondaryAccounts, name);
  }
}
} // namespace sbash64::budget
