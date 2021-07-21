#include "budget.hpp"
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
                   const Transaction &transaction) {
  account->credit(transaction);
}

static void debit(const std::shared_ptr<Account> &account,
                  const Transaction &transaction) {
  account->debit(transaction);
}

static void verifyCredit(const std::shared_ptr<Account> &account,
                         const Transaction &transaction) {
  account->verifyCredit(transaction);
}

static void verifyDebit(const std::shared_ptr<Account> &account,
                        const Transaction &transaction) {
  account->verifyDebit(transaction);
}

static void removeCredit(const std::shared_ptr<Account> &account,
                         const Transaction &transaction) {
  account->removeCredit(transaction);
}

static void removeDebit(const std::shared_ptr<Account> &account,
                        const Transaction &transaction) {
  account->removeDebit(transaction);
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

static auto make(Account::Factory &accountFactory, std::string_view name,
                 Budget::Observer *observer) -> std::shared_ptr<Account> {
  auto account{accountFactory.make(name)};
  callIfObserverExists(observer, [&](Budget::Observer *observer_) {
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
    Account::Factory &accountFactory, std::string_view accountName,
    Budget::Observer *observer) {
  if (!contains(accounts, accountName))
    accounts[std::string{accountName}] =
        make(accountFactory, accountName, observer);
}

static auto collect(const std::map<std::string, std::shared_ptr<Account>,
                                   std::less<>> &accounts)
    -> std::vector<SerializableAccount *> {
  std::vector<SerializableAccount *> collected;
  transform(
      accounts.begin(), accounts.end(), back_inserter(collected),
      [](const std::pair<const std::string, std::shared_ptr<Account>> &pair) {
        return pair.second.get();
      });
  return collected;
}

static auto
at(const std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
   std::string_view name) -> const std::shared_ptr<Account> & {
  return accounts.at(std::string{name});
}

static auto makeAndLoad(Account::Factory &factory,
                        AccountDeserialization &deserialization,
                        std::string_view name, Budget::Observer *observer)
    -> std::shared_ptr<Account> {
  auto account{make(factory, name, observer)};
  account->load(deserialization);
  return account;
}

BudgetInMemory::BudgetInMemory(Account::Factory &accountFactory)
    : accountFactory{accountFactory},
      primaryAccount{make(accountFactory, masterAccountName.data(), observer)} {
}

void BudgetInMemory::attach(Observer *a) { observer = a; }

void BudgetInMemory::credit(const Transaction &transaction) {
  budget::credit(primaryAccount, transaction);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void BudgetInMemory::removeCredit(const Transaction &transaction) {
  budget::removeCredit(primaryAccount, transaction);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void BudgetInMemory::debit(std::string_view accountName,
                           const Transaction &transaction) {
  createNewAccountIfNeeded(secondaryAccounts, accountFactory, accountName,
                           observer);
  budget::debit(at(secondaryAccounts, accountName), transaction);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void BudgetInMemory::removeDebit(std::string_view accountName,
                                 const Transaction &transaction) {
  if (contains(secondaryAccounts, accountName)) {
    budget::removeDebit(at(secondaryAccounts, accountName), transaction);
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
  createNewAccountIfNeeded(secondaryAccounts, accountFactory, accountName,
                           observer);
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

static void
remove(std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
       std::string_view name) {
  at(accounts, name)->remove();
  accounts.erase(std::string{name});
}

void BudgetInMemory::load(BudgetDeserialization &persistentMemory) {

  primaryAccount->remove();
  for (auto [name, account] : secondaryAccounts)
    account->remove();
  primaryAccount.reset();
  secondaryAccounts.clear();
  persistentMemory.load(*this);
  notifyThatTotalBalanceHasChanged(observer, primaryAccount, secondaryAccounts);
}

void BudgetInMemory::renameAccount(std::string_view from, std::string_view to) {
  at(secondaryAccounts, from)->rename(to);
}

void BudgetInMemory::verifyDebit(std::string_view accountName,
                                 const Transaction &transaction) {
  budget::verifyDebit(at(secondaryAccounts, accountName), transaction);
}

void BudgetInMemory::verifyCredit(const Transaction &transaction) {
  budget::verifyCredit(primaryAccount, transaction);
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
  primaryAccount = makeAndLoad(accountFactory, deserialization, name, observer);
}

void BudgetInMemory::notifyThatSecondaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  secondaryAccounts[std::string{name}] =
      makeAndLoad(accountFactory, deserialization, name, observer);
}

void BudgetInMemory::reduce(const Date &date) {
  primaryAccount->reduce(date);
  for (const auto &[name, account] : secondaryAccounts)
    account->reduce(date);
}

void BudgetInMemory::createAccount(std::string_view name) {
  createNewAccountIfNeeded(secondaryAccounts, accountFactory, name, observer);
}

static auto transaction(USD amount, const std::stringstream &description,
                        const Date &date) -> Transaction {
  return {amount, description.str(), date};
}

void BudgetInMemory::closeAccount(std::string_view name, const Date &date) {
  if (contains(secondaryAccounts, name)) {
    std::stringstream description;
    description << "close " << name;
    const auto balance{at(secondaryAccounts, name)->balance()};
    if (balance.cents > 0) {
      budget::credit(primaryAccount, transaction(balance, description, date));
      budget::verifyCredit(primaryAccount,
                           transaction(balance, description, date));
    } else if (balance.cents < 0) {
      budget::debit(primaryAccount, transaction(-balance, description, date));
      budget::verifyDebit(primaryAccount,
                          transaction(-balance, description, date));
    }
    remove(secondaryAccounts, name);
  }
}
} // namespace sbash64::budget
