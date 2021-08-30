#include "budget.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <sstream>
#include <string_view>

namespace sbash64::budget {
static void add(Account &account, const Transaction &transaction) {
  account.add(transaction);
}

static void add(const std::shared_ptr<Account> &account,
                const Transaction &transaction) {
  account->add(transaction);
}

static void verify(Account &account, const Transaction &transaction) {
  account.verify(transaction);
}

static void verify(const std::shared_ptr<Account> &account,
                   const Transaction &transaction) {
  account->verify(transaction);
}

static void remove(Account &account, const Transaction &transaction) {
  account.remove(transaction);
}

static void remove(const std::shared_ptr<Account> &account,
                   const Transaction &transaction) {
  account->remove(transaction);
}

static void
callIfObserverExists(BudgetInMemory::Observer *observer,
                     const std::function<void(BudgetInMemory::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
}

static void notifyThatNetIncomeHasChanged(
    BudgetInMemory::Observer *observer, Account &incomeAccount,
    const std::map<std::string, std::shared_ptr<Account>, std::less<>>
        &expenseAccounts) {
  callIfObserverExists(observer, [&](BudgetInMemory::Observer *observer_) {
    observer_->notifyThatNetIncomeHasChanged(accumulate(
        expenseAccounts.begin(), expenseAccounts.end(), incomeAccount.balance(),
        [](USD total, const std::pair<std::string_view,
                                      std::shared_ptr<Account>> &secondary) {
          const auto &[name, account] = secondary;
          return total + account->balance();
        }));
  });
}

static auto makeExpenseAccount(Account::Factory &accountFactory,
                               std::string_view name,
                               std::map<std::string, USD> &categoryAllocations,
                               Budget::Observer *observer)
    -> std::shared_ptr<Account> {
  auto account{accountFactory.make(name)};
  categoryAllocations[std::string{name}] = USD{0};
  callIfObserverExists(observer, [&](Budget::Observer *observer_) {
    observer_->notifyThatExpenseAccountHasBeenCreated(*account, name);
  });
  return account;
}

static auto
contains(std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
         std::string_view accountName) -> bool {
  return accounts.count(accountName) != 0;
}

static void createExpenseAccountIfNeeded(
    std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
    Account::Factory &accountFactory, std::string_view accountName,
    std::map<std::string, USD> &categoryAllocations,
    Budget::Observer *observer) {
  if (!contains(accounts, accountName))
    accounts[std::string{accountName}] = makeExpenseAccount(
        accountFactory, accountName, categoryAllocations, observer);
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

static auto makeAndLoadExpenseAccount(
    Account::Factory &factory, AccountDeserialization &deserialization,
    std::string_view name, std::map<std::string, USD> &categoryAllocations,
    Budget::Observer *observer) -> std::shared_ptr<Account> {
  auto account{
      makeExpenseAccount(factory, name, categoryAllocations, observer)};
  account->load(deserialization);
  return account;
}

BudgetInMemory::BudgetInMemory(Account &incomeAccount,
                               Account::Factory &accountFactory)
    : accountFactory{accountFactory}, incomeAccount{incomeAccount} {}

void BudgetInMemory::attach(Observer *a) { observer = a; }

void BudgetInMemory::addIncome(const Transaction &transaction) {
  budget::add(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::addExpense(std::string_view accountName,
                                const Transaction &transaction) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               categoryAllocations, observer);
  budget::add(at(expenseAccounts, accountName), transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::removeIncome(const Transaction &transaction) {
  budget::remove(incomeAccount, transaction);
  notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
}

void BudgetInMemory::removeExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  if (contains(expenseAccounts, accountName)) {
    budget::remove(at(expenseAccounts, accountName), transaction);
    notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
  }
}

void BudgetInMemory::verifyIncome(const Transaction &transaction) {
  budget::verify(incomeAccount, transaction);
}

void BudgetInMemory::verifyExpense(std::string_view accountName,
                                   const Transaction &transaction) {
  budget::verify(at(expenseAccounts, accountName), transaction);
}

static void transferTo(Account &incomeAccount,
                       const std::map<std::string, std::shared_ptr<Account>,
                                      std::less<>> &expenseAccounts,
                       std::string_view accountName, USD amount) {
  incomeAccount.withdraw(amount);
  at(expenseAccounts, accountName)->deposit(amount);
}

static void notifyThatCategoryAllocationHasChanged(
    Budget::Observer *observer, std::string_view name,
    const std::map<std::string, USD> &categoryAllocations) {
  observer->notifyThatCategoryAllocationHasChanged(
      name, categoryAllocations.at(std::string{name}));
}

void BudgetInMemory::transferTo(std::string_view accountName, USD amount) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               categoryAllocations, observer);
  budget::transferTo(incomeAccount, expenseAccounts, accountName, amount);
  categoryAllocations.at(std::string{accountName}) += amount;
  callIfObserverExists(observer, [&](Observer *observer_) {
    notifyThatCategoryAllocationHasChanged(observer_, accountName,
                                           categoryAllocations);
  });
}

static void transfer(Account &from, Account &to, USD amount) {
  from.withdraw(amount);
  to.deposit(amount);
}

void BudgetInMemory::allocate(std::string_view accountName, USD amountNeeded) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, accountName,
                               categoryAllocations, observer);
  const auto amount{amountNeeded -
                    categoryAllocations.at(std::string{accountName})};
  categoryAllocations.at(std::string{accountName}) = amountNeeded;
  callIfObserverExists(observer, [&](Observer *observer_) {
    if (unallocatedIncome.cents > amount.cents)
      observer_->notifyThatUnallocatedIncomeHasChanged(unallocatedIncome -
                                                       amount);
    else if (unallocatedIncome.cents < amount.cents)
      observer_->notifyThatIncomeDeficitHasChanged(amount - unallocatedIncome);
    notifyThatCategoryAllocationHasChanged(observer_, accountName,
                                           categoryAllocations);
  });
  unallocatedIncome -= amount;
}

void BudgetInMemory::createAccount(std::string_view name) {
  createExpenseAccountIfNeeded(expenseAccounts, accountFactory, name,
                               categoryAllocations, observer);
}

static void
remove(std::map<std::string, std::shared_ptr<Account>, std::less<>> &accounts,
       std::string_view name) {
  at(accounts, name)->remove();
  accounts.erase(std::string{name});
}

void BudgetInMemory::closeAccount(std::string_view name) {
  if (contains(expenseAccounts, name)) {
    const auto balance{at(expenseAccounts, name)->balance()};
    const auto leftover{categoryAllocations.at(std::string{name}) - balance};
    callIfObserverExists(observer, [&](Observer *observer_) {
      if (unallocatedIncome.cents + leftover.cents > 0)
        observer_->notifyThatUnallocatedIncomeHasChanged(unallocatedIncome +
                                                         leftover);
      else if (unallocatedIncome.cents + leftover.cents < 0)
        observer_->notifyThatIncomeDeficitHasChanged(-leftover -
                                                     unallocatedIncome);
    });
    unallocatedIncome += leftover;
    remove(expenseAccounts, name);
  }
}

void BudgetInMemory::renameAccount(std::string_view from, std::string_view to) {
  at(expenseAccounts, from)->rename(to);
}

void BudgetInMemory::removeAccount(std::string_view name) {
  if (contains(expenseAccounts, name)) {
    remove(expenseAccounts, name);
    notifyThatNetIncomeHasChanged(observer, incomeAccount, expenseAccounts);
  }
}

void BudgetInMemory::save(BudgetSerialization &persistentMemory) {
  persistentMemory.save(incomeAccount, collect(expenseAccounts));
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
  expenseAccounts[std::string{name}] = makeAndLoadExpenseAccount(
      accountFactory, deserialization, name, categoryAllocations, observer);
}

void BudgetInMemory::reduce() {
  incomeAccount.reduce();
  for (const auto &[name, account] : expenseAccounts)
    account->reduce();
}

void BudgetInMemory::restore() {
  for (auto [name, account] : expenseAccounts)
    if (account->balance().cents < 0)
      budget::transferTo(incomeAccount, expenseAccounts, name,
                         -account->balance());
}
} // namespace sbash64::budget
