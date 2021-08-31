#include "account.hpp"

#include <functional>
#include <numeric>

namespace sbash64::budget {
static void
callIfObserverExists(Account::Observer *observer,
                     const std::function<void(Account::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
}

static auto
balance(const std::vector<std::shared_ptr<ObservableTransaction>> &transactions)
    -> USD {
  return accumulate(
      transactions.begin(), transactions.end(), USD{0},
      [](USD total, const std::shared_ptr<ObservableTransaction> &t) {
        return total + t->amount();
      });
}

static void verify(
    const Transaction &t,
    std::vector<std::shared_ptr<ObservableTransaction>> &transactionRecords) {
  for (const auto &record : transactionRecords)
    if (record->verifies(t))
      return;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer)
    -> std::shared_ptr<ObservableTransaction> {
  auto transactionRecord{factory.make()};
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatHasBeenAdded(*transactionRecord);
  });
  return transactionRecord;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer, const Transaction &t)
    -> std::shared_ptr<ObservableTransaction> {
  auto transactionRecord{make(factory, observer)};
  transactionRecord->initialize(t);
  return transactionRecord;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer,
                 TransactionDeserialization &deserialization)
    -> std::shared_ptr<ObservableTransaction> {
  auto transactionRecord{make(factory, observer)};
  deserialization.load(*transactionRecord);
  return transactionRecord;
}

static void notifyUpdatedBalance(Account &account,
                                 Account::Observer *observer) {
  callIfObserverExists(observer, [&](InMemoryAccount::Observer *observer_) {
    observer_->notifyThatBalanceHasChanged(account.balance());
  });
}

static void add(std::vector<std::shared_ptr<ObservableTransaction>> &records,
                ObservableTransaction::Factory &factory, Account &account,
                Account::Observer *observer, const Transaction &transaction) {
  records.push_back(make(factory, observer, transaction));
  notifyUpdatedBalance(account, observer);
}

static void
addTransaction(std::vector<std::shared_ptr<ObservableTransaction>> &records,
               ObservableTransaction::Factory &factory, Account &account,
               Account::Observer *observer,
               TransactionDeserialization &deserialization) {
  records.push_back(make(factory, observer, deserialization));
  notifyUpdatedBalance(account, observer);
}

static void remove(std::vector<std::shared_ptr<ObservableTransaction>> &records,
                   Account &account, Account::Observer *observer,
                   const Transaction &transaction) {
  if (const auto found = find_if(
          records.begin(), records.end(),
          [&transaction](const std::shared_ptr<ObservableTransaction> &record) {
            return record->removes(transaction);
          });
      found != records.end()) {
    records.erase(found);
    notifyUpdatedBalance(account, observer);
  }
}

InMemoryAccount::InMemoryAccount(ObservableTransaction::Factory &factory)
    : factory{factory} {}

void InMemoryAccount::attach(Observer *a) { observer = a; }

void InMemoryAccount::add(const Transaction &transaction) {
  budget::add(transactions, factory, *this, observer, transaction);
}

void InMemoryAccount::remove(const Transaction &transaction) {
  budget::remove(transactions, *this, observer, transaction);
}

void InMemoryAccount::verify(const Transaction &transaction) {
  budget::verify(transaction, transactions);
}

static auto
collect(const std::vector<std::shared_ptr<ObservableTransaction>> &accounts)
    -> std::vector<SerializableTransaction *> {
  std::vector<SerializableTransaction *> collected;
  collected.reserve(accounts.size());
  for (const auto &account : accounts)
    collected.push_back(account.get());
  return collected;
}

void InMemoryAccount::save(AccountSerialization &serialization) {
  serialization.save(collect(transactions));
}

void InMemoryAccount::load(AccountDeserialization &deserialization) {
  deserialization.load(*this);
}

void InMemoryAccount::notifyThatIsReady(
    TransactionDeserialization &deserialization) {
  addTransaction(transactions, factory, *this, observer, deserialization);
}

static void
clear(std::vector<std::shared_ptr<ObservableTransaction>> &records) {
  for (const auto &record : records)
    record->remove();
  records.clear();
}

void InMemoryAccount::reduce() { budget::clear(transactions); }

auto InMemoryAccount::balance() -> USD { return budget::balance(transactions); }

void InMemoryAccount::remove() {
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatWillBeRemoved();
  });
}

void InMemoryAccount::clear() {
  budget::clear(transactions);
  notifyUpdatedBalance(*this, observer);
}

auto InMemoryAccount::Factory::make() -> std::shared_ptr<Account> {
  return std::make_shared<InMemoryAccount>(transactionFactory);
}

InMemoryAccount::Factory::Factory(
    ObservableTransaction::Factory &transactionFactory)
    : transactionFactory{transactionFactory} {}
} // namespace sbash64::budget
