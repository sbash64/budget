#include "account.hpp"
#include "domain.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>

namespace sbash64::budget {
static auto balance(const AccountInMemory::TransactionsType &transactions)
    -> USD {
  return accumulate(transactions.begin(), transactions.end(), USD{0},
                    [](USD total, const auto &transaction) {
                      return total + transaction->amount();
                    });
}

static void verify(const Transaction &toVerify,
                   AccountInMemory::TransactionsType &transactions) {
  for (const auto &transaction : transactions)
    if (transaction->verifies(toVerify))
      return;
}

static auto
make(ObservableTransaction::Factory &factory,
     const std::vector<std::reference_wrapper<Account::Observer>> &observers)
    -> std::shared_ptr<ObservableTransaction> {
  auto transaction{factory.make()};
  for (auto observer : observers)
    observer.get().notifyThatHasBeenAdded(*transaction);
  return transaction;
}

static auto
make(ObservableTransaction::Factory &factory,
     const std::vector<std::reference_wrapper<Account::Observer>> &observer,
     const Transaction &transaction) -> std::shared_ptr<ObservableTransaction> {
  auto made{make(factory, observer)};
  made->initialize(transaction);
  return made;
}

static auto
make(ObservableTransaction::Factory &factory,
     const std::vector<std::reference_wrapper<Account::Observer>> &observer,
     TransactionDeserialization &deserialization)
    -> std::shared_ptr<ObservableTransaction> {
  auto transaction{make(factory, observer)};
  deserialization.load(*transaction);
  return transaction;
}

static void notifyUpdatedBalance(
    const AccountInMemory::TransactionsType &transactions,
    const std::vector<std::reference_wrapper<Account::Observer>> &observers) {
  for (auto observer : observers)
    observer.get().notifyThatBalanceHasChanged(balance(transactions));
}

static void
add(AccountInMemory::TransactionsType &transactions,
    ObservableTransaction::Factory &factory,
    const std::vector<std::reference_wrapper<Account::Observer>> &observer,
    const Transaction &transaction) {
  transactions.push_back(make(factory, observer, transaction));
  notifyUpdatedBalance(transactions, observer);
}

static void addTransaction(
    AccountInMemory::TransactionsType &transactions,
    ObservableTransaction::Factory &factory,
    const std::vector<std::reference_wrapper<Account::Observer>> &observer,
    TransactionDeserialization &deserialization) {
  transactions.push_back(make(factory, observer, deserialization));
  notifyUpdatedBalance(transactions, observer);
}

static void
remove(AccountInMemory::TransactionsType &transactions,
       const std::vector<std::reference_wrapper<Account::Observer>> &observer,
       const Transaction &toRemove) {
  if (const auto found = std::find_if(transactions.begin(), transactions.end(),
                                      [&toRemove](const auto &transaction) {
                                        return transaction->removes(toRemove);
                                      });
      found != transactions.end()) {
    transactions.erase(found);
    notifyUpdatedBalance(transactions, observer);
  }
}

static void clear(AccountInMemory::TransactionsType &records) {
  for (const auto &record : records)
    record->remove();
  records.clear();
}

static void notifyUpdatedAllocation(
    const std::vector<std::reference_wrapper<Account::Observer>> &observers,
    USD allocation) {
  for (auto observer : observers)
    observer.get().notifyThatAllocationHasChanged(allocation);
}

static void resolveVerifiedTransactions(
    AccountInMemory::TransactionsType &transactions,
    AccountInMemory::TransactionsType &archived, USD &allocation,
    const std::function<void(USD &, const std::shared_ptr<ObservableTransaction>
                                        &)> &updateAllocation,
    const std::vector<std::reference_wrapper<Account::Observer>> &observers) {
  const auto firstNotVerified{std::partition(
      transactions.begin(), transactions.end(),
      [](const auto &transaction) { return transaction->verified(); })};
  std::for_each(transactions.begin(), firstNotVerified,
                [&](const auto &transaction) {
                  updateAllocation(allocation, transaction);
                  transaction->archive();
                });
  archived.insert(archived.end(), std::make_move_iterator(transactions.begin()),
                  std::make_move_iterator(firstNotVerified));
  transactions.erase(transactions.begin(), firstNotVerified);
  notifyUpdatedAllocation(observers, allocation);
  notifyUpdatedBalance(transactions, observers);
}

static auto collect(const AccountInMemory::TransactionsType &transactions,
                    const AccountInMemory::TransactionsType &archived)
    -> std::vector<SerializableTransaction *> {
  std::vector<SerializableTransaction *> collected;
  collected.reserve(transactions.size() + archived.size());
  for (const auto &account : transactions)
    collected.push_back(account.get());
  for (const auto &account : archived)
    collected.push_back(account.get());
  return collected;
}

AccountInMemory::AccountInMemory(ObservableTransaction::Factory &factory)
    : factory{factory} {}

void AccountInMemory::attach(Observer &a) { observers.push_back(std::ref(a)); }

void AccountInMemory::add(const Transaction &transaction) {
  budget::add(transactions, factory, observers, transaction);
}

void AccountInMemory::remove(const Transaction &transaction) {
  budget::remove(transactions, observers, transaction);
}

void AccountInMemory::verify(const Transaction &transaction) {
  budget::verify(transaction, transactions);
}

void AccountInMemory::save(AccountSerialization &serialization) {
  serialization.save(collect(transactions, archived), allocation);
}

void AccountInMemory::load(AccountDeserialization &deserialization) {
  deserialization.load(*this);
}

void AccountInMemory::notifyThatIsReady(
    TransactionDeserialization &deserialization) {
  addTransaction(transactions, factory, observers, deserialization);
}

void AccountInMemory::increaseAllocationByResolvingVerifiedTransactions() {
  resolveVerifiedTransactions(
      transactions, archived, allocation,
      [](USD &allocation_,
         const std::shared_ptr<ObservableTransaction> &transaction) {
        allocation_ += transaction->amount();
      },
      observers);
}

void AccountInMemory::decreaseAllocationByResolvingVerifiedTransactions() {
  resolveVerifiedTransactions(
      transactions, archived, allocation,
      [](USD &allocation_,
         const std::shared_ptr<ObservableTransaction> &transaction) {
        allocation_ -= transaction->amount();
      },
      observers);
}

auto AccountInMemory::balance() -> USD { return budget::balance(transactions); }

void AccountInMemory::rename(std::string_view name) {
  for (auto observer : observers)
    observer.get().notifyThatNameHasChanged(name);
}

void AccountInMemory::remove() {
  for (auto observer : observers)
    observer.get().notifyThatWillBeRemoved();
}

void AccountInMemory::clear() {
  budget::clear(transactions);
  budget::clear(archived);
  allocation.cents = 0;
  notifyUpdatedAllocation(observers, allocation);
  notifyUpdatedBalance(transactions, observers);
}

void AccountInMemory::increaseAllocationBy(USD usd) {
  allocation += usd;
  notifyUpdatedAllocation(observers, allocation);
}

void AccountInMemory::decreaseAllocationBy(USD usd) {
  allocation -= usd;
  notifyUpdatedAllocation(observers, allocation);
}

auto AccountInMemory::allocated() -> USD { return allocation; }

void AccountInMemory::notifyThatAllocatedIsReady(USD usd) {
  allocation = usd;
  notifyUpdatedAllocation(observers, allocation);
}

auto AccountInMemory::Factory::make() -> std::shared_ptr<Account> {
  return std::make_shared<AccountInMemory>(transactionFactory);
}

AccountInMemory::Factory::Factory(
    ObservableTransaction::Factory &transactionFactory)
    : transactionFactory{transactionFactory} {}
} // namespace sbash64::budget
