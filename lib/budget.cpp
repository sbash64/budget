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

static void notifyThatNetIncomeHasChanged(
    BudgetInMemory::Observer *observer, Account &incomeAccount,
    const std::map<std::string, AccountWithAllocation, std::less<>>
        &expenseAccountsWithAllocations) {
  callIfObserverExists(observer, [&](BudgetInMemory::Observer *observer_) {
    observer_->notifyThatNetIncomeHasChanged(accumulate(
        expenseAccountsWithAllocations.begin(),
        expenseAccountsWithAllocations.end(), incomeAccount.balance(),
        [](USD net, const std::pair<std::string_view, AccountWithAllocation>
                        &expenseAccountWithAllocation) {
          const auto &[name, accountWithAllocation] =
              expenseAccountWithAllocation;
          return net - accountWithAllocation.account->balance();
        }));
  });
}

static auto
contains(std::map<std::string, AccountWithAllocation, std::less<>> &accounts,
         std::string_view accountName) -> bool {
  return accounts.count(accountName) != 0;
}

static auto collect(
    const std::map<std::string, AccountWithAllocation, std::less<>> &accounts)
    -> std::vector<SerializableAccountWithFundsAndName> {
  std::vector<SerializableAccountWithFundsAndName> collected;
  transform(accounts.begin(), accounts.end(), back_inserter(collected),
            [&](const std::pair<const std::string, AccountWithAllocation>
                    &expenseAccountWithAllocation) {
              const auto &[name, accountWithAllocation] =
                  expenseAccountWithAllocation;
              return SerializableAccountWithFundsAndName{
                  accountWithAllocation.account.get(),
                  accountWithAllocation.allocation, name};
            });
  return collected;
}

static auto at(const std::map<std::string, AccountWithAllocation, std::less<>>
                   &accountsWithAllocation,
               std::string_view name) -> const std::shared_ptr<Account> & {
  return accountsWithAllocation.at(std::string{name}).account;
}

static void
makeExpenseAccount(std::map<std::string, AccountWithAllocation, std::less<>>
                       &accountsWithAllocation,
                   Account::Factory &accountFactory, std::string_view name,
                   Budget::Observer *observer) {
  accountsWithAllocation[std::string{name}] = {accountFactory.make(name),
                                               USD{0}};
  callIfObserverExists(observer, [&](Budget::Observer *observer_) {
    observer_->notifyThatExpenseAccountHasBeenCreated(
        *at(accountsWithAllocation, name), name);
  });
}

static void makeAndLoadExpenseAccount(
    std::map<std::string, AccountWithAllocation, std::less<>>
        &accountsWithAllocation,
    Account::Factory &factory, AccountDeserialization &deserialization,
    std::string_view name, Budget::Observer *observer) {
  makeExpenseAccount(accountsWithAllocation, factory, name, observer);
  at(accountsWithAllocation, name)->load(deserialization);
}

static void createExpenseAccountIfNeeded(
    std::map<std::string, AccountWithAllocation, std::less<>>
        &accountsWithAllocation,
    Account::Factory &accountFactory, std::string_view accountName,
    Budget::Observer *observer) {
  if (!contains(accountsWithAllocation, accountName))
    makeExpenseAccount(accountsWithAllocation, accountFactory, accountName,
                       observer);
}

BudgetInMemory::BudgetInMemory(Account &incomeAccount,
                               Account::Factory &accountFactory)
    : accountFactory{accountFactory}, incomeAccount{incomeAccount} {}

void BudgetInMemory::attach(Observer *a) { observer = a; }

void BudgetInMemory::addIncome(const Transaction &transaction) {
  budget::add(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount,
                                expenseAccountsWithAllocations);
}

void BudgetInMemory::addExpense(std::string_view accountName,
                                const Transaction &transaction) {
  createExpenseAccountIfNeeded(expenseAccountsWithAllocations, accountFactory,
                               accountName, observer);
  budget::add(at(expenseAccountsWithAllocations, accountName), transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount,
                                expenseAccountsWithAllocations);
}

void BudgetInMemory::removeIncome(const Transaction &transaction) {
  budget::remove(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount,
                                expenseAccountsWithAllocations);
}

void BudgetInMemory::removeExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  if (contains(expenseAccountsWithAllocations, accountName)) {
    budget::remove(at(expenseAccountsWithAllocations, accountName),
                   transaction);
    notifyThatNetIncomeHasChanged(observer, incomeAccount,
                                  expenseAccountsWithAllocations);
  }
}

void BudgetInMemory::verifyIncome(const Transaction &transaction) {
  budget::verify(incomeAccount, transaction);
}

void BudgetInMemory::verifyExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  budget::verify(at(expenseAccountsWithAllocations, accountName), transaction);
}

static void notifyThatCategoryAllocationHasChanged(
    Budget::Observer *observer, std::string_view name,
    const std::map<std::string, AccountWithAllocation, std::less<>>
        &accountsWithAllocation) {
  observer->notifyThatCategoryAllocationHasChanged(
      name, accountsWithAllocation.at(std::string{name}).allocation);
}

static void transfer(std::map<std::string, AccountWithAllocation, std::less<>>
                         &accountsWithAllocation,
                     USD &unallocatedIncome, std::string_view name, USD amount,
                     Budget::Observer *observer) {
  accountsWithAllocation.at(std::string{name}).allocation += amount;
  unallocatedIncome -= amount;
  callIfObserverExists(observer, [&](Budget::Observer *observer_) {
    observer_->notifyThatUnallocatedIncomeHasChanged(unallocatedIncome);
    notifyThatCategoryAllocationHasChanged(observer_, name,
                                           accountsWithAllocation);
  });
}

void BudgetInMemory::transferTo(std::string_view accountName, USD amount) {
  createExpenseAccountIfNeeded(expenseAccountsWithAllocations, accountFactory,
                               accountName, observer);
  transfer(expenseAccountsWithAllocations, unallocatedIncome, accountName,
           amount, observer);
}

void BudgetInMemory::allocate(std::string_view accountName, USD amountNeeded) {
  createExpenseAccountIfNeeded(expenseAccountsWithAllocations, accountFactory,
                               accountName, observer);
  transfer(expenseAccountsWithAllocations, unallocatedIncome, accountName,
           amountNeeded -
               expenseAccountsWithAllocations.at(std::string{accountName})
                   .allocation,
           observer);
}

void BudgetInMemory::createAccount(std::string_view name) {
  createExpenseAccountIfNeeded(expenseAccountsWithAllocations, accountFactory,
                               name, observer);
}

static void remove(std::map<std::string, AccountWithAllocation, std::less<>>
                       &accountsWithAllocation,
                   std::string_view name) {
  at(accountsWithAllocation, name)->remove();
  accountsWithAllocation.erase(std::string{name});
}

void BudgetInMemory::closeAccount(std::string_view name) {
  if (contains(expenseAccountsWithAllocations, name)) {
    unallocatedIncome +=
        expenseAccountsWithAllocations.at(std::string{name}).allocation -
        at(expenseAccountsWithAllocations, name)->balance();
    callIfObserverExists(observer, [&](Observer *observer_) {
      observer_->notifyThatUnallocatedIncomeHasChanged(unallocatedIncome);
    });
    remove(expenseAccountsWithAllocations, name);
  }
}

void BudgetInMemory::renameAccount(std::string_view from, std::string_view to) {
  auto fromNode2{expenseAccountsWithAllocations.extract(std::string{from})};
  fromNode2.key() = to;
  expenseAccountsWithAllocations.insert(std::move(fromNode2));
}

void BudgetInMemory::removeAccount(std::string_view name) {
  if (contains(expenseAccountsWithAllocations, name)) {
    remove(expenseAccountsWithAllocations, name);
    notifyThatNetIncomeHasChanged(observer, incomeAccount,
                                  expenseAccountsWithAllocations);
  }
}

void BudgetInMemory::save(BudgetSerialization &persistentMemory) {
  persistentMemory.save({&incomeAccount, unallocatedIncome},
                        collect(expenseAccountsWithAllocations));
}

void BudgetInMemory::load(BudgetDeserialization &persistentMemory) {
  for (auto [name, accountWithAllocation] : expenseAccountsWithAllocations)
    accountWithAllocation.account->remove();
  incomeAccount.clear();
  expenseAccountsWithAllocations.clear();
  persistentMemory.load(*this);
  notifyThatNetIncomeHasChanged(observer, incomeAccount,
                                expenseAccountsWithAllocations);
}

void BudgetInMemory::notifyThatIncomeAccountIsReady(
    AccountDeserialization &deserialization, USD) {
  incomeAccount.load(deserialization);
}

void BudgetInMemory::notifyThatExpenseAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name, USD) {
  makeAndLoadExpenseAccount(expenseAccountsWithAllocations, accountFactory,
                            deserialization, name, observer);
}

void BudgetInMemory::reduce() {
  unallocatedIncome += incomeAccount.balance();
  callIfObserverExists(observer, [&](Observer *observer_) {
    observer_->notifyThatUnallocatedIncomeHasChanged(unallocatedIncome);
  });
  incomeAccount.reduce();
  for (auto &[name, accountWithAllocation] : expenseAccountsWithAllocations) {
    accountWithAllocation.allocation -=
        accountWithAllocation.account->balance();
    std::string copied{name};
    callIfObserverExists(observer, [&](Observer *observer_) {
      notifyThatCategoryAllocationHasChanged(observer_, copied,
                                             expenseAccountsWithAllocations);
    });
    accountWithAllocation.account->reduce();
  }
}

void BudgetInMemory::restore() {
  for (auto [name, accountWithAllocation] : expenseAccountsWithAllocations) {
    const auto amount{accountWithAllocation.allocation -
                      at(expenseAccountsWithAllocations, name)->balance()};
    if (amount.cents < 0)
      transfer(expenseAccountsWithAllocations, unallocatedIncome, name, -amount,
               observer);
  }
}
} // namespace sbash64::budget
