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

static auto net(const AccountWithAllocation &accountWithAllocation) -> USD {
  return accountWithAllocation.allocation -
         accountWithAllocation.account->balance();
}

static void notifyThatNetIncomeHasChanged(
    BudgetInMemory::Observer *observer,
    StaticAccountWithAllocation incomeAccountWithAllocation,
    const std::map<std::string, AccountWithAllocation, std::less<>>
        &expenseAccountsWithAllocations) {
  callIfObserverExists(observer, [&](BudgetInMemory::Observer *observer_) {
    observer_->notifyThatNetIncomeHasChanged(accumulate(
        expenseAccountsWithAllocations.begin(),
        expenseAccountsWithAllocations.end(),
        incomeAccountWithAllocation.account.balance() +
            incomeAccountWithAllocation.allocation,
        [](USD net, const std::pair<std::string, AccountWithAllocation>
                        &expenseAccountWithAllocation) {
          const auto &[name, accountWithAllocation] =
              expenseAccountWithAllocation;
          return net + budget::net(accountWithAllocation);
        }));
  });
}

static auto
contains(std::map<std::string, AccountWithAllocation, std::less<>> &accounts,
         std::string_view accountName) -> bool {
  return accounts.contains(accountName);
}

static auto collect(const std::map<std::string, AccountWithAllocation,
                                   std::less<>> &accountsWithAllocation)
    -> std::vector<SerializableAccountWithFundsAndName> {
  std::vector<SerializableAccountWithFundsAndName> collected;
  transform(accountsWithAllocation.begin(), accountsWithAllocation.end(),
            back_inserter(collected),
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
  return accountsWithAllocation.find(name)->second.account;
}

static void
makeExpenseAccount(std::map<std::string, AccountWithAllocation, std::less<>>
                       &accountsWithAllocation,
                   Account::Factory &accountFactory, std::string_view name,
                   Budget::Observer *observer) {
  accountsWithAllocation.insert(std::make_pair(
      name, AccountWithAllocation{accountFactory.make(), USD{0}}));
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
    : incomeAccountWithAllocation{incomeAccount, USD{0}}, accountFactory{
                                                              accountFactory} {}

void BudgetInMemory::attach(Observer *a) { observer = a; }

void BudgetInMemory::addIncome(const Transaction &transaction) {
  budget::add(incomeAccountWithAllocation.account, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccountWithAllocation,
                                expenseAccountsWithAllocations);
}

void BudgetInMemory::addExpense(std::string_view accountName,
                                const Transaction &transaction) {
  createExpenseAccountIfNeeded(expenseAccountsWithAllocations, accountFactory,
                               accountName, observer);
  budget::add(at(expenseAccountsWithAllocations, accountName), transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccountWithAllocation,
                                expenseAccountsWithAllocations);
}

void BudgetInMemory::removeIncome(const Transaction &transaction) {
  budget::remove(incomeAccountWithAllocation.account, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccountWithAllocation,
                                expenseAccountsWithAllocations);
}

void BudgetInMemory::removeExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  if (contains(expenseAccountsWithAllocations, accountName)) {
    budget::remove(at(expenseAccountsWithAllocations, accountName),
                   transaction);
    notifyThatNetIncomeHasChanged(observer, incomeAccountWithAllocation,
                                  expenseAccountsWithAllocations);
  }
}

void BudgetInMemory::verifyIncome(const Transaction &transaction) {
  budget::verify(incomeAccountWithAllocation.account, transaction);
}

void BudgetInMemory::verifyExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  budget::verify(at(expenseAccountsWithAllocations, accountName), transaction);
}

static void notifyThatCategoryAllocationHasChanged(
    Budget::Observer *observer, std::string_view name,
    const std::map<std::string, AccountWithAllocation, std::less<>>
        &accountsWithAllocation) {
  callIfObserverExists(observer, [&](Budget::Observer *observer_) {
    observer_->notifyThatCategoryAllocationHasChanged(
        name, accountsWithAllocation.find(name)->second.allocation);
  });
}

static void transfer(std::map<std::string, AccountWithAllocation, std::less<>>
                         &accountsWithAllocation,
                     USD &unallocatedIncome, std::string_view name, USD amount,
                     Budget::Observer *observer) {
  accountsWithAllocation.find(name)->second.allocation += amount;
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
  transfer(expenseAccountsWithAllocations,
           incomeAccountWithAllocation.allocation, accountName, amount,
           observer);
}

void BudgetInMemory::allocate(std::string_view accountName, USD amountNeeded) {
  createExpenseAccountIfNeeded(expenseAccountsWithAllocations, accountFactory,
                               accountName, observer);
  transfer(
      expenseAccountsWithAllocations, incomeAccountWithAllocation.allocation,
      accountName,
      amountNeeded -
          expenseAccountsWithAllocations.find(accountName)->second.allocation,
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
    incomeAccountWithAllocation.allocation +=
        expenseAccountsWithAllocations.find(name)->second.allocation -
        at(expenseAccountsWithAllocations, name)->balance();
    callIfObserverExists(observer, [&](Observer *observer_) {
      observer_->notifyThatUnallocatedIncomeHasChanged(
          incomeAccountWithAllocation.allocation);
    });
    remove(expenseAccountsWithAllocations, name);
  }
}

void BudgetInMemory::renameAccount(std::string_view from, std::string_view to) {
  auto fromNode{expenseAccountsWithAllocations.extract(std::string{from})};
  fromNode.key() = to;
  expenseAccountsWithAllocations.insert(std::move(fromNode));
}

void BudgetInMemory::removeAccount(std::string_view name) {
  if (contains(expenseAccountsWithAllocations, name)) {
    remove(expenseAccountsWithAllocations, name);
    notifyThatNetIncomeHasChanged(observer, incomeAccountWithAllocation,
                                  expenseAccountsWithAllocations);
  }
}

void BudgetInMemory::save(BudgetSerialization &persistentMemory) {
  persistentMemory.save({&incomeAccountWithAllocation.account,
                         incomeAccountWithAllocation.allocation},
                        collect(expenseAccountsWithAllocations));
}

void BudgetInMemory::load(BudgetDeserialization &persistentMemory) {
  for (auto [name, accountWithAllocation] : expenseAccountsWithAllocations)
    accountWithAllocation.account->remove();
  incomeAccountWithAllocation.account.clear();
  expenseAccountsWithAllocations.clear();
  persistentMemory.load(*this);
  notifyThatNetIncomeHasChanged(observer, incomeAccountWithAllocation,
                                expenseAccountsWithAllocations);
}

void BudgetInMemory::notifyThatIncomeAccountIsReady(
    AccountDeserialization &deserialization, USD unallocated) {
  incomeAccountWithAllocation.account.load(deserialization);
  incomeAccountWithAllocation.allocation = unallocated;
  callIfObserverExists(observer, [&](Observer *observer_) {
    observer_->notifyThatUnallocatedIncomeHasChanged(
        incomeAccountWithAllocation.allocation);
  });
}

void BudgetInMemory::notifyThatExpenseAccountIsReady(
    AccountDeserialization &deserialization, std::string_view name,
    USD allocated) {
  makeAndLoadExpenseAccount(expenseAccountsWithAllocations, accountFactory,
                            deserialization, name, observer);
  expenseAccountsWithAllocations.find(name)->second.allocation = allocated;
  notifyThatCategoryAllocationHasChanged(observer, name,
                                         expenseAccountsWithAllocations);
}

void BudgetInMemory::reduce() {
  incomeAccountWithAllocation.allocation +=
      incomeAccountWithAllocation.account.balance();
  callIfObserverExists(observer, [&](Observer *observer_) {
    observer_->notifyThatUnallocatedIncomeHasChanged(
        incomeAccountWithAllocation.allocation);
  });
  incomeAccountWithAllocation.account.reduce();
  for (auto &[name, accountWithAllocation] : expenseAccountsWithAllocations) {
    accountWithAllocation.allocation -=
        accountWithAllocation.account->balance();
    std::string_view view{name};
    callIfObserverExists(observer, [&](Observer *observer_) {
      notifyThatCategoryAllocationHasChanged(observer_, view,
                                             expenseAccountsWithAllocations);
    });
    accountWithAllocation.account->reduce();
  }
}

void BudgetInMemory::restore() {
  for (auto [name, accountWithAllocation] : expenseAccountsWithAllocations) {
    const auto amount{net(accountWithAllocation)};
    if (amount.cents < 0)
      transfer(expenseAccountsWithAllocations,
               incomeAccountWithAllocation.allocation, name, -amount, observer);
  }
}
} // namespace sbash64::budget
