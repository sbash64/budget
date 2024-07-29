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
  try {
    account.remove(transaction);
  } catch (const Account::TransactionNotFound &) {
  }
}

static auto leftoverAfterExpenses(Account &account) -> USD {
  return account.allocated() - account.balance();
}

static void notifyThatNetIncomeHasChanged(
    const std::vector<std::reference_wrapper<BudgetInMemory::Observer>>
        &observers,
    Account &incomeAccount,
    const BudgetInMemory::ExpenseAccountsType &expenseAccounts) {
  for (auto observer : observers)
    observer.get().notifyThatNetIncomeHasChanged(
        accumulate(expenseAccounts.begin(), expenseAccounts.end(),
                   incomeAccount.balance() + incomeAccount.allocated(),
                   [](USD net, const auto &expenseAccount) {
                     const auto &[name, account] = expenseAccount;
                     return net + leftoverAfterExpenses(*account);
                   }));
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

static void makeExpenseAccount(
    BudgetInMemory::ExpenseAccountsType &expenseAccounts,
    Account::Factory &accountFactory, std::string_view name,
    const std::vector<std::reference_wrapper<BudgetInMemory::Observer>>
        &observers) {
  expenseAccounts.insert(std::make_pair(name, accountFactory.make()));
  for (auto observer : observers)
    observer.get().notifyThatExpenseAccountHasBeenCreated(
        *at(expenseAccounts, name), name);
}

static void makeAndLoadExpenseAccount(
    BudgetInMemory::ExpenseAccountsType &expenseAccounts,
    Account::Factory &factory, AccountDeserialization &deserialization,
    std::string_view name,
    const std::vector<std::reference_wrapper<BudgetInMemory::Observer>>
        &observer) {
  makeExpenseAccount(expenseAccounts, factory, name, observer);
  at(expenseAccounts, name)->load(deserialization);
}

static void notifyThatHasUnsavedChanges(
    const std::vector<std::reference_wrapper<BudgetInMemory::Observer>>
        &observers) {
  for (auto observer : observers)
    observer.get().notifyThatHasUnsavedChanges();
}

static void createExpenseAccountIfNeeded(
    BudgetInMemory::ExpenseAccountsType &expenseAccounts,
    Account::Factory &accountFactory, std::string_view accountName,
    const std::vector<std::reference_wrapper<BudgetInMemory::Observer>>
        &observer) {
  if (!contains(expenseAccounts, accountName)) {
    makeExpenseAccount(expenseAccounts, accountFactory, accountName, observer);
    notifyThatHasUnsavedChanges(observer);
  }
}

BudgetInMemory::BudgetInMemory(Account &incomeAccount,
                               Account::Factory &accountFactory)
    : incomeAccount{incomeAccount}, accountFactory{accountFactory} {}

void BudgetInMemory::attach(Observer &a) { observers.push_back(std::ref(a)); }

void BudgetInMemory::addIncome(const Transaction &transaction) {
  add(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observers, incomeAccount, expenseAccounts);
  notifyThatHasUnsavedChanges(observers);
}

void BudgetInMemory::addExpense(std::string_view accountName,
                                const Transaction &transaction) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observers);
  add(at(expenseAccounts, accountName), transaction);
  notifyThatNetIncomeHasChanged(observers, incomeAccount, expenseAccounts);
  notifyThatHasUnsavedChanges(observers);
}

void BudgetInMemory::removeIncome(const Transaction &transaction) {
  remove(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observers, incomeAccount, expenseAccounts);
  notifyThatHasUnsavedChanges(observers);
}

void BudgetInMemory::removeExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  if (contains(expenseAccounts, accountName)) {
    remove(*at(expenseAccounts, accountName), transaction);
    notifyThatNetIncomeHasChanged(observers, incomeAccount, expenseAccounts);
    notifyThatHasUnsavedChanges(observers);
  }
}

void BudgetInMemory::verifyIncome(const Transaction &transaction) {
  verify(incomeAccount, transaction);
  notifyThatHasUnsavedChanges(observers);
}

void BudgetInMemory::verifyExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  if (contains(expenseAccounts, accountName)) {
    verify(at(expenseAccounts, accountName), transaction);
    notifyThatHasUnsavedChanges(observers);
  }
}

static void transfer(Account &from, Account &to, USD amount) {
  from.decreaseAllocationBy(amount);
  to.increaseAllocationBy(amount);
}

static void
transfer(BudgetInMemory::ExpenseAccountsType &expenseAccounts,
         std::string_view name, Account &from, USD amount,
         const std::vector<std::reference_wrapper<BudgetInMemory::Observer>>
             &observer) {
  transfer(from, *at(expenseAccounts, name), amount);
  notifyThatHasUnsavedChanges(observer);
}

void BudgetInMemory::transferTo(std::string_view accountName, USD amount) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observers);
  transfer(expenseAccounts, accountName, incomeAccount, amount, observers);
}

void BudgetInMemory::allocate(std::string_view accountName, USD amountNeeded) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observers);
  const auto amount{amountNeeded - allocation(expenseAccounts, accountName)};
  if (amount.cents > 0)
    transfer(expenseAccounts, accountName, incomeAccount, amount, observers);
  else if (amount.cents < 0)
    transfer(*at(expenseAccounts, accountName), incomeAccount, -amount);
}

void BudgetInMemory::createAccount(std::string_view name) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, name,
                               observers);
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
    notifyThatHasUnsavedChanges(observers);
  }
}

void BudgetInMemory::renameAccount(std::string_view from, std::string_view to) {
  if (contains(expenseAccounts, to))
    return;
  auto node{expenseAccounts.extract(std::string{from})};
  if (node.empty())
    return;
  node.key() = to;
  node.mapped()->rename(to);
  expenseAccounts.insert(std::move(node));
  notifyThatHasUnsavedChanges(observers);
}

void BudgetInMemory::removeAccount(std::string_view name) {
  if (contains(expenseAccounts, name)) {
    remove(expenseAccounts, name);
    notifyThatNetIncomeHasChanged(observers, incomeAccount, expenseAccounts);
    notifyThatHasUnsavedChanges(observers);
  }
}

void BudgetInMemory::save(BudgetSerialization &persistentMemory) {
  persistentMemory.save(&incomeAccount, collect(expenseAccounts));
  for (auto observer : observers)
    observer.get().notifyThatHasBeenSaved();
}

void BudgetInMemory::load(BudgetDeserialization &persistentMemory) {
  for (const auto &[name, account] : expenseAccounts)
    account->remove();
  incomeAccount.clear();
  expenseAccounts.clear();
  persistentMemory.load(*this);
  notifyThatNetIncomeHasChanged(observers, incomeAccount, expenseAccounts);
}

void BudgetInMemory::notifyThatIncomeAccountIsReady(
    AccountDeserialization &deserialization) {
  incomeAccount.load(deserialization);
}

void BudgetInMemory::notifyThatExpenseAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name) {
  makeAndLoadExpenseAccount(expenseAccounts, accountFactory, deserialization,
                            name, observers);
}

void BudgetInMemory::reduce() {
  incomeAccount.increaseAllocationByResolvingVerifiedTransactions();
  for (const auto &[name, account] : expenseAccounts)
    account->decreaseAllocationByResolvingVerifiedTransactions();
  notifyThatHasUnsavedChanges(observers);
}

void BudgetInMemory::restore() {
  for (const auto &[name, account] : expenseAccounts) {
    const auto amount{leftoverAfterExpenses(*account)};
    if (amount.cents < 0)
      transfer(expenseAccounts, name, incomeAccount, -amount, observers);
  }
}
} // namespace sbash64::budget
