#include "budget.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
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

static auto leftoverAfterExpenses(Account &accountWithAllocation) -> USD {
  return accountWithAllocation.allocated() - accountWithAllocation.balance();
}

static void notifyThatNetIncomeHasChanged(
    BudgetInMemory::Observer *observer, Account &incomeAccountWithAllocation,
    const std::map<std::string, std::shared_ptr<Account>, std::less<>>
        &expenseAccounts) {
  callIfObserverExists(observer, [&](BudgetInMemory::Observer *observer_) {
    observer_->notifyThatNetIncomeHasChanged(accumulate(
        expenseAccounts.begin(), expenseAccounts.end(),
        incomeAccountWithAllocation.balance() +
            incomeAccountWithAllocation.allocated(),
        [](USD net, const std::pair<std::string, std::shared_ptr<Account>>
                        &expenseAccountWithAllocation) {
          return net +
                 leftoverAfterExpenses(*expenseAccountWithAllocation.second);
        }));
  });
}

static auto
contains(std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
         std::string_view accountName) -> bool {
  return accounts.find(accountName) != accounts.end();
}

static auto collect(const std::map<std::string, std::shared_ptr<Account>,
                                   std::less<>> &expenseAccountsWithAllocation)
    -> std::vector<SerializableAccountWithName> {
  std::vector<SerializableAccountWithName> collected;
  transform(
      expenseAccountsWithAllocation.begin(),
      expenseAccountsWithAllocation.end(), back_inserter(collected),
      [&](const std::pair<std::string, std::shared_ptr<Account>>
              &expenseAccountWithAllocation) {
        const auto &[name, accountWithAllocation] =
            expenseAccountWithAllocation;
        return SerializableAccountWithName{accountWithAllocation.get(), name};
      });
  return collected;
}

static auto at(const std::map<std::string, std::shared_ptr<Account>,
                              std::less<>> &accountsWithAllocation,
               std::string_view name) -> const std::shared_ptr<Account> & {
  return accountsWithAllocation.find(name)->second;
}

static auto account(const std::map<std::string, std::shared_ptr<Account>,
                                   std::less<>> &accountsWithAllocation,
                    std::string_view name) -> const std::shared_ptr<Account> & {
  return at(accountsWithAllocation, name);
}

static auto allocation(const std::map<std::string, std::shared_ptr<Account>,
                                      std::less<>> &accountsWithAllocation,
                       std::string_view name) -> USD {
  return at(accountsWithAllocation, name)->allocated();
}

static void
makeExpenseAccount(std::map<std::string, std::shared_ptr<Account>, std::less<>>
                       &accountsWithAllocation,
                   Account::Factory &accountFactory, std::string_view name,
                   Budget::Observer *observer) {
  accountsWithAllocation.insert(std::make_pair(name, accountFactory.make()));
  callIfObserverExists(
      observer, [&accountsWithAllocation, name](Budget::Observer *observer_) {
        observer_->notifyThatExpenseAccountHasBeenCreated(
            *account(accountsWithAllocation, name), name);
      });
}

static void makeAndLoadExpenseAccount(
    std::map<std::string, std::shared_ptr<Account>, std::less<>>
        &accountsWithAllocation,
    Account::Factory &factory, AccountDeserialization &deserialization,
    std::string_view name, Budget::Observer *observer) {
  makeExpenseAccount(accountsWithAllocation, factory, name, observer);
  account(accountsWithAllocation, name)->load(deserialization);
}

static void createExpenseAccountIfNeeded(
    std::map<std::string, std::shared_ptr<Account>, std::less<>>
        &accountsWithAllocation,
    Account::Factory &accountFactory, std::string_view accountName,
    Budget::Observer *observer) {
  if (!contains(accountsWithAllocation, accountName))
    makeExpenseAccount(accountsWithAllocation, accountFactory, accountName,
                       observer);
}

BudgetInMemory::BudgetInMemory(Account &incomeAccount,
                               Account::Factory &accountFactory)
    : incomeAccount{incomeAccount}, accountFactory{accountFactory} {}

void BudgetInMemory::attach(Observer *a) { observer = a; }

void BudgetInMemory::addIncome(const Transaction &transaction) {
  add(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::addExpense(std::string_view accountName,
                                const Transaction &transaction) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observer);
  add(at(expenseAccounts, accountName), transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
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

static void transfer(std::map<std::string, std::shared_ptr<Account>,
                              std::less<>> &expenseAccounts,
                     Account &from, std::string_view name, USD amount) {
  transfer(from, *at(expenseAccounts, name), amount);
}

void BudgetInMemory::transferTo(std::string_view accountName, USD amount) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observer);
  transfer(expenseAccounts, incomeAccount, accountName, amount);
}

void BudgetInMemory::allocate(std::string_view accountName, USD amountNeeded) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               observer);
  const auto amount{amountNeeded - allocation(expenseAccounts, accountName)};
  if (amount.cents > 0)
    transfer(incomeAccount, *at(expenseAccounts, accountName), amount);
  else if (amount.cents < 0)
    transfer(*at(expenseAccounts, accountName), incomeAccount, -amount);
}

void BudgetInMemory::createAccount(std::string_view name) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, name, observer);
}

static void remove(std::map<std::string, std::shared_ptr<Account>, std::less<>>
                       &accountsWithAllocation,
                   std::string_view name) {
  account(accountsWithAllocation, name)->remove();
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
}

void BudgetInMemory::load(BudgetDeserialization &persistentMemory) {
  for (auto [name, accountWithAllocation] : expenseAccounts)
    accountWithAllocation->remove();
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
  for (auto &[name, accountWithAllocation] : expenseAccounts) {
    accountWithAllocation->decreaseAllocationByResolvingVerifiedTransactions();
  }
}

void BudgetInMemory::restore() {
  for (auto [name, accountWithAllocation] : expenseAccounts) {
    const auto amount{leftoverAfterExpenses(*accountWithAllocation)};
    if (amount.cents < 0)
      transfer(expenseAccounts, incomeAccount, name, -amount);
  }
}
} // namespace sbash64::budget
