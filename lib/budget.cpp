#include "budget.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <string_view>
#include <utility>

namespace sbash64::budget {
static void add(Account &account, const Transaction &transaction) {
  account.add(transaction);
}

static void add(const std::shared_ptr<Account> &account,
                const Transaction &transaction) {
  add(*account, transaction);
}

static void verify(Account &account, const Transaction &transaction) {
  account.verify(transaction);
}

static void verify(const std::shared_ptr<Account> &account,
                   const Transaction &transaction) {
  verify(*account, transaction);
}

static void remove(Account &account, const Transaction &transaction) {
  account.remove(transaction);
}

static void remove(const std::shared_ptr<Account> &account,
                   const Transaction &transaction) {
  remove(*account, transaction);
}

static void
callIfObserverExists(BudgetInMemory::Observer *observer,
                     const std::function<void(BudgetInMemory::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
}

static auto leftoverAfterExpenses(Account &account) -> USD {
  return account.allocated() - account.balance();
}

static void notifyThatNetIncomeHasChanged(
    BudgetInMemory::Observer *observer, Account &incomeAccount,
    const BudgetInMemory::ExpenseAccountsType &expenseAccounts) {
  callIfObserverExists(observer, [&](BudgetInMemory::Observer *observer_) {
    observer_->notifyThatNetIncomeHasChanged(
        accumulate(expenseAccounts.begin(), expenseAccounts.end(),
                   incomeAccount.balance() + incomeAccount.allocated(),
                   [](USD net, const auto &expenseAccount) {
                     const auto &[name, account] = expenseAccount;
                     return net + leftoverAfterExpenses(*account);
                   }));
  });
}

static auto contains(BudgetInMemory::ExpenseAccountsType &accounts,
                     std::string_view accountName) -> bool {
  return accounts.find(accountName) != accounts.end();
}

static auto collect(const BudgetInMemory::ExpenseAccountsType &expenseAccounts)
    -> std::vector<SerializableAccountWithName> {
  std::vector<SerializableAccountWithName> collected;
  transform(expenseAccounts.begin(), expenseAccounts.end(),
            back_inserter(collected), [&](const auto &expenseAccount) {
              const auto &[name, account] = expenseAccount;
              return SerializableAccountWithName{account.get(), name};
            });
  return collected;
}

static auto at(const BudgetInMemory::ExpenseAccountsType &expenseAccounts,
               std::string_view name) -> const std::shared_ptr<Account> & {
  return expenseAccounts.find(name)->second;
}

static auto
allocation(const BudgetInMemory::ExpenseAccountsType &expenseAccounts,
           std::string_view name) -> USD {
  return at(expenseAccounts, name)->allocated();
}

static void
makeExpenseAccount(BudgetInMemory::ExpenseAccountsType &expenseAccounts,
                   Account::Factory &accountFactory, std::string_view name,
                   Budget::Observer *observer) {
  expenseAccounts.insert(std::make_pair(name, accountFactory.make()));
  callIfObserverExists(observer,
                       [&expenseAccounts, name](Budget::Observer *observer_) {
                         observer_->notifyThatExpenseAccountHasBeenCreated(
                             *at(expenseAccounts, name), name);
                       });
}

static void
makeAndLoadExpenseAccount(BudgetInMemory::ExpenseAccountsType &expenseAccounts,
                          Account::Factory &factory,
                          AccountDeserialization &deserialization,
                          std::string_view name, Budget::Observer *observer) {
  makeExpenseAccount(expenseAccounts, factory, name, observer);
  at(expenseAccounts, name)->load(deserialization);
}

static void createExpenseAccountIfNeeded(
    BudgetInMemory::ExpenseAccountsType &expenseAccounts,
    Account::Factory &accountFactory, std::string_view accountName,
    Budget::Observer *observer) {
  if (!contains(expenseAccounts, accountName))
    makeExpenseAccount(expenseAccounts, accountFactory, accountName, observer);
}

static void notifyThatHasUnsavedChanges(BudgetInMemory::Observer *observer) {
  callIfObserverExists(observer, [](BudgetInMemory::Observer *observer_) {
    observer_->notifyThatHasUnsavedChanges();
  });
}

BudgetInMemory::BudgetInMemory(Account &incomeAccount,
                               Account::Factory &accountFactory)
    : incomeAccount{incomeAccount}, accountFactory{accountFactory} {}

void BudgetInMemory::attach(Observer *a) { observer = a; }

void BudgetInMemory::addIncome(const Transaction &transaction) {
  add(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
  notifyThatHasUnsavedChanges(observer);
}

void BudgetInMemory::addExpense(std::string_view accountName,
                                const Transaction &transaction) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observer);
  add(at(expenseAccounts, accountName), transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
  notifyThatHasUnsavedChanges(observer);
}

void BudgetInMemory::removeIncome(const Transaction &transaction) {
  remove(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::removeExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  if (contains(expenseAccounts, accountName)) {
    remove(at(expenseAccounts, accountName), transaction);
    notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
    notifyThatHasUnsavedChanges(observer);
  }
}

void BudgetInMemory::verifyIncome(const Transaction &transaction) {
  verify(incomeAccount, transaction);
}

void BudgetInMemory::verifyExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  verify(at(expenseAccounts, accountName), transaction);
}

static void transfer(Account &from, Account &to, USD amount) {
  from.decreaseAllocationBy(amount);
  to.increaseAllocationBy(amount);
}

static void transfer(BudgetInMemory::ExpenseAccountsType &expenseAccounts,
                     std::string_view name, Account &from, USD amount) {
  transfer(from, *at(expenseAccounts, name), amount);
}

void BudgetInMemory::transferTo(std::string_view accountName, USD amount) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observer);
  transfer(expenseAccounts, accountName, incomeAccount, amount);
  notifyThatHasUnsavedChanges(observer);
}

void BudgetInMemory::allocate(std::string_view accountName, USD amountNeeded) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observer);
  const auto amount{amountNeeded - allocation(expenseAccounts, accountName)};
  if (amount.cents > 0)
    transfer(expenseAccounts, accountName, incomeAccount, amount);
  else if (amount.cents < 0)
    transfer(*at(expenseAccounts, accountName), incomeAccount, -amount);
}

void BudgetInMemory::createAccount(std::string_view name) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, name, observer);
}

static void remove(BudgetInMemory::ExpenseAccountsType &accountsWithAllocation,
                   std::string_view name) {
  at(accountsWithAllocation, name)->remove();
  accountsWithAllocation.erase(std::string{name});
}

void BudgetInMemory::closeAccount(std::string_view name) {
  if (contains(expenseAccounts, name)) {
    const auto amount{leftoverAfterExpenses(*at(expenseAccounts, name))};
    if (amount.cents > 0)
      incomeAccount.increaseAllocationBy(amount);
    else if (amount.cents < 0)
      incomeAccount.decreaseAllocationBy(-amount);
    remove(expenseAccounts, name);
  }
}

void BudgetInMemory::renameAccount(std::string_view from, std::string_view to) {
  auto fromNode{expenseAccounts.extract(std::string{from})};
  fromNode.key() = to;
  expenseAccounts.insert(std::move(fromNode));
}

void BudgetInMemory::removeAccount(std::string_view name) {
  if (contains(expenseAccounts, name)) {
    remove(expenseAccounts, name);
    notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
  }
}

void BudgetInMemory::save(BudgetSerialization &persistentMemory) {
  persistentMemory.save(&incomeAccount, collect(expenseAccounts));
  callIfObserverExists(observer, [](BudgetInMemory::Observer *observer_) {
    observer_->notifyThatHasBeenSaved();
  });
}

void BudgetInMemory::load(BudgetDeserialization &persistentMemory) {
  for (auto [name, account] : expenseAccounts)
    account->remove();
  incomeAccount.clear();
  expenseAccounts.clear();
  persistentMemory.load(*this);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::notifyThatIncomeAccountIsReady(
    AccountDeserialization &deserialization) {
  incomeAccount.load(deserialization);
}

void BudgetInMemory::notifyThatExpenseAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  makeAndLoadExpenseAccount(expenseAccounts, accountFactory, deserialization,
                            name, observer);
}

void BudgetInMemory::reduce() {
  incomeAccount.increaseAllocationByResolvingVerifiedTransactions();
  for (auto &[name, account] : expenseAccounts)
    account->decreaseAllocationByResolvingVerifiedTransactions();
}

void BudgetInMemory::restore() {
  for (auto [name, account] : expenseAccounts) {
    const auto amount{leftoverAfterExpenses(*account)};
    if (amount.cents < 0)
      transfer(expenseAccounts, name, incomeAccount, -amount);
  }
}
} // namespace sbash64::budget
