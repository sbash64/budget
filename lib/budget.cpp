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

static void credit(Account &account, const Transaction &transaction) {
  account.credit(transaction);
}

static void credit(const std::shared_ptr<Account> &account,
                   const Transaction &transaction) {
  account->credit(transaction);
}

static void debit(Account &account, const Transaction &transaction) {
  account.debit(transaction);
}

static void debit(const std::shared_ptr<Account> &account,
                  const Transaction &transaction) {
  account->debit(transaction);
}

static void verifyCredit(Account &account, const Transaction &transaction) {
  account.verifyCredit(transaction);
}

static void verifyCredit(const std::shared_ptr<Account> &account,
                         const Transaction &transaction) {
  account->verifyCredit(transaction);
}

static void verifyDebit(Account &account, const Transaction &transaction) {
  account.verifyDebit(transaction);
}

static void verifyDebit(const std::shared_ptr<Account> &account,
                        const Transaction &transaction) {
  account->verifyDebit(transaction);
}

static void removeCredit(Account &account, const Transaction &transaction) {
  account.removeCredit(transaction);
}

static void removeCredit(const std::shared_ptr<Account> &account,
                         const Transaction &transaction) {
  account->removeCredit(transaction);
}

static void removeDebit(Account &account, const Transaction &transaction) {
  account.removeDebit(transaction);
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
    BudgetInMemory::Observer *observer, Account &primaryAccount,
    const std::map<std::string, std::shared_ptr<Account>, std::less<>>
        &secondaryAccounts) {
  callIfObserverExists(observer, [&](BudgetInMemory::Observer *observer_) {
    observer_->notifyThatTotalBalanceHasChanged(accumulate(
        secondaryAccounts.begin(), secondaryAccounts.end(),
        primaryAccount.balance(),
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

BudgetInMemory::BudgetInMemory(Account &primaryAccount,
                               Account::Factory &accountFactory)
    : accountFactory{accountFactory}, incomeAccount{primaryAccount} {}

void BudgetInMemory::attach(Observer *a) { observer = a; }

void BudgetInMemory::credit(const Transaction &transaction) {
  budget::credit(incomeAccount, transaction);
  notifyThatTotalBalanceHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::removeCredit(const Transaction &transaction) {
  budget::removeCredit(incomeAccount, transaction);
  notifyThatTotalBalanceHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::debit(std::string_view accountName,
                           const Transaction &transaction) {
  createNewAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                           observer);
  budget::debit(at(expenseAccounts, accountName), transaction);
  notifyThatTotalBalanceHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::removeDebit(std::string_view accountName,
                                 const Transaction &transaction) {
  if (contains(expenseAccounts, accountName)) {
    budget::removeDebit(at(expenseAccounts, accountName), transaction);
    notifyThatTotalBalanceHasChanged(observer, incomeAccount, expenseAccounts);
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

static void transferTo(Account &primaryAccount,
                       const std::map<std::string, std::shared_ptr<Account>,
                                      std::less<>> &secondaryAccounts,
                       std::string_view accountName, USD amount) {
  primaryAccount.withdraw(amount);
  at(secondaryAccounts, accountName)->deposit(amount);
}

void BudgetInMemory::transferTo(std::string_view accountName, USD amount) {
  createNewAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                           observer);
  budget::transferTo(incomeAccount, expenseAccounts, accountName, amount);
}

void BudgetInMemory::save(BudgetSerialization &persistentMemory) {
  persistentMemory.save(incomeAccount, collect(expenseAccounts));
}

static void
remove(std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
       std::string_view name) {
  at(accounts, name)->remove();
  accounts.erase(std::string{name});
}

void BudgetInMemory::load(BudgetDeserialization &persistentMemory) {
  for (auto [name, account] : expenseAccounts)
    account->remove();
  incomeAccount.clear();
  expenseAccounts.clear();
  persistentMemory.load(*this);
  notifyThatTotalBalanceHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::renameAccount(std::string_view from, std::string_view to) {
  at(expenseAccounts, from)->rename(to);
}

void BudgetInMemory::verifyDebit(std::string_view accountName,
                                 const Transaction &transaction) {
  budget::verifyDebit(at(expenseAccounts, accountName), transaction);
}

void BudgetInMemory::verifyCredit(const Transaction &transaction) {
  budget::verifyCredit(incomeAccount, transaction);
}

void BudgetInMemory::removeAccount(std::string_view name) {
  if (contains(expenseAccounts, name)) {
    remove(expenseAccounts, name);
    notifyThatTotalBalanceHasChanged(observer, incomeAccount, expenseAccounts);
  }
}

void BudgetInMemory::notifyThatPrimaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view) {
  incomeAccount.load(deserialization);
}

void BudgetInMemory::notifyThatSecondaryAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  expenseAccounts[std::string{name}] =
      makeAndLoad(accountFactory, deserialization, name, observer);
}

void BudgetInMemory::reduce(const Date &date) {
  incomeAccount.reduce(date);
  for (const auto &[name, account] : expenseAccounts)
    account->reduce(date);
}

void BudgetInMemory::createAccount(std::string_view name) {
  createNewAccountIfNeeded(expenseAccounts, accountFactory, name, observer);
}

static auto transaction(USD amount, const std::stringstream &description,
                        const Date &date) -> Transaction {
  return {amount, description.str(), date};
}

void BudgetInMemory::closeAccount(std::string_view name) {
  if (contains(expenseAccounts, name)) {
    const auto balance{at(expenseAccounts, name)->balance()};
    if (balance.cents > 0) {
      incomeAccount.deposit(balance);
    } else if (balance.cents < 0) {
      incomeAccount.withdraw(-balance);
    }
    remove(expenseAccounts, name);
  }
}

void BudgetInMemory::allocate(std::string_view accountName, USD amountNeeded) {
  createNewAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                           observer);
  const auto amount{amountNeeded - at(expenseAccounts, accountName)->balance()};

  if (amount.cents > 0) {
    incomeAccount.withdraw(amount);
    at(expenseAccounts, accountName)->deposit(amount);
  } else if (amount.cents < 0) {
    at(expenseAccounts, accountName)->withdraw(-amount);
    incomeAccount.deposit(-amount);
  }
}

void BudgetInMemory::restore(const Date &) {
  for (auto [name, account] : expenseAccounts)
    if (account->balance().cents < 0)
      budget::transferTo(incomeAccount, expenseAccounts, name,
                         -account->balance());
}
} // namespace sbash64::budget
