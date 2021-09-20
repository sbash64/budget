#include "account.hpp"
#include "domain.hpp"

#include <functional>
#include <numeric>

namespace sbash64::budget {
static void
callIfObserverExists(Account::Observer *observer,
                     const std::function<void(Account::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
}

static auto balance(const AccountInMemory::TransactionsType &transactions)
    -> USD {
  return accumulate(transactions.begin(), transactions.end(), USD{0},
                    [](USD total, const auto &transaction) {
                      return total + (transaction->archived()
                                          ? USD{0}
                                          : transaction->amount());
                    });
}

static void verify(const Transaction &toVerify,
                   AccountInMemory::TransactionsType &transactions) {
  for (const auto &transaction : transactions)
    if (transaction->verifies(toVerify))
      return;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer)
    -> std::shared_ptr<ObservableTransaction> {
  auto transaction{factory.make()};
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatHasBeenAdded(*transaction);
  });
  return transaction;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer, const Transaction &transaction)
    -> std::shared_ptr<ObservableTransaction> {
  auto made{make(factory, observer)};
  made->initialize(transaction);
  return made;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer,
                 TransactionDeserialization &deserialization)
    -> std::shared_ptr<ObservableTransaction> {
  auto transaction{make(factory, observer)};
  deserialization.load(*transaction);
  return transaction;
}

static void
notifyUpdatedBalance(const AccountInMemory::TransactionsType &transactions,
                     Account::Observer *observer) {
  callIfObserverExists(observer, [&](AccountInMemory::Observer *observer_) {
    observer_->notifyThatBalanceHasChanged(balance(transactions));
  });
}

static void add(AccountInMemory::TransactionsType &transactions,
                ObservableTransaction::Factory &factory,
                Account::Observer *observer, const Transaction &transaction) {
  transactions.push_back(make(factory, observer, transaction));
  notifyUpdatedBalance(transactions, observer);
}

static void addTransaction(AccountInMemory::TransactionsType &transactions,
                           ObservableTransaction::Factory &factory,
                           Account::Observer *observer,
                           TransactionDeserialization &deserialization) {
  transactions.push_back(make(factory, observer, deserialization));
  notifyUpdatedBalance(transactions, observer);
}

static void remove(AccountInMemory::TransactionsType &transactions,
                   Account::Observer *observer, const Transaction &toRemove) {
  if (const auto found = find_if(transactions.begin(), transactions.end(),
                                 [&toRemove](const auto &transaction) {
                                   return transaction->removes(toRemove);
                                 });
      found != transactions.end()) {
    transactions.erase(found);
    notifyUpdatedBalance(transactions, observer);
  }
}

AccountInMemory::AccountInMemory(ObservableTransaction::Factory &factory)
    : factory{factory} {}

void AccountInMemory::attach(Observer *a) { observer = a; }

void AccountInMemory::add(const Transaction &transaction) {
  budget::add(transactions, factory, observer, transaction);
}

void AccountInMemory::remove(const Transaction &transaction) {
  budget::remove(transactions, observer, transaction);
}

void AccountInMemory::verify(const Transaction &transaction) {
  budget::verify(transaction, transactions);
}

static auto collect(const AccountInMemory::TransactionsType &accounts)
    -> std::vector<SerializableTransaction *> {
  std::vector<SerializableTransaction *> collected;
  collected.reserve(accounts.size());
  for (const auto &account : accounts)
    collected.push_back(account.get());
  return collected;
}

void AccountInMemory::save(AccountSerialization &serialization) {
  serialization.save(collect(transactions), USD{});
}

void AccountInMemory::load(AccountDeserialization &deserialization) {
  deserialization.load(*this);
}

void AccountInMemory::notifyThatIsReady(
    TransactionDeserialization &deserialization) {
  addTransaction(transactions, factory, observer, deserialization);
}

static void clear(AccountInMemory::TransactionsType &records) {
  for (const auto &record : records)
    record->remove();
  records.clear();
}

static void resolveVerifiedTransactions(
    const AccountInMemory::TransactionsType &transactions, USD &allocation,
    const std::function<void(USD &, const std::shared_ptr<ObservableTransaction>
                                        &)> &updateAllocation,
    Account::Observer *observer) {
  for (const auto &transaction : transactions)
    if (transaction->verified()) {
      updateAllocation(allocation, transaction);
      transaction->archive();
    }
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatAllocationHasChanged(allocation);
  });
  notifyUpdatedBalance(transactions, observer);
}

void AccountInMemory::increaseAllocationByResolvingVerifiedTransactions() {
  resolveVerifiedTransactions(
      transactions, allocation,
      [](USD &allocation_,
         const std::shared_ptr<ObservableTransaction> &transaction) {
        allocation_ += transaction->amount();
      },
      observer);
}

void AccountInMemory::decreaseAllocationByResolvingVerifiedTransactions() {
  resolveVerifiedTransactions(
      transactions, allocation,
      [](USD &allocation_,
         const std::shared_ptr<ObservableTransaction> &transaction) {
        allocation_ -= transaction->amount();
      },
      observer);
}

auto AccountInMemory::balance() -> USD { return budget::balance(transactions); }

void AccountInMemory::remove() {
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatWillBeRemoved();
  });
}

void AccountInMemory::clear() {
  budget::clear(transactions);
  notifyUpdatedBalance(transactions, observer);
}

void AccountInMemory::increaseAllocationBy(USD) {}

void AccountInMemory::decreaseAllocationBy(USD) {}

auto AccountInMemory::allocated() -> USD { return {}; }

void AccountInMemory::notifyThatAllocatedIsReady(USD) {}

auto AccountInMemory::Factory::make() -> std::shared_ptr<Account> {
  return std::make_shared<AccountInMemory>(transactionFactory);
}

AccountInMemory::Factory::Factory(
    ObservableTransaction::Factory &transactionFactory)
    : transactionFactory{transactionFactory} {}
} // namespace sbash64::budget
