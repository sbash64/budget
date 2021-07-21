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

static void callIfObserverExists(
    ObservableTransaction::Observer *observer,
    const std::function<void(ObservableTransaction::Observer *)> &f) {
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

static auto
balance(const std::vector<std::shared_ptr<ObservableTransaction>> &credits,
        const std::vector<std::shared_ptr<ObservableTransaction>> &debits)
    -> USD {
  return balance(credits) - balance(debits);
}

static void verify(
    const Transaction &t,
    std::vector<std::shared_ptr<ObservableTransaction>> &transactionRecords) {
  for (const auto &record : transactionRecords)
    if (record->verifies(t))
      return;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer,
                 void (Account::Observer::*notify)(ObservableTransaction &))
    -> std::shared_ptr<ObservableTransaction> {
  auto transactionRecord{factory.make()};
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    (observer_->*notify)(*transactionRecord);
  });
  return transactionRecord;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer,
                 void (Account::Observer::*notify)(ObservableTransaction &),
                 const Transaction &t)
    -> std::shared_ptr<ObservableTransaction> {
  auto transactionRecord{make(factory, observer, notify)};
  transactionRecord->initialize(t);
  return transactionRecord;
}

static auto make(ObservableTransaction::Factory &factory,
                 Account::Observer *observer,
                 void (Account::Observer::*notify)(ObservableTransaction &),
                 TransactionDeserialization &deserialization)
    -> std::shared_ptr<ObservableTransaction> {
  auto transactionRecord{make(factory, observer, notify)};
  deserialization.load(*transactionRecord);
  return transactionRecord;
}

static void notifyUpdatedBalance(
    Account::Observer *observer,
    const std::vector<std::shared_ptr<ObservableTransaction>> &creditRecords,
    const std::vector<std::shared_ptr<ObservableTransaction>> &debitRecords) {
  callIfObserverExists(observer, [&](InMemoryAccount::Observer *observer_) {
    observer_->notifyThatBalanceHasChanged(
        budget::balance(creditRecords, debitRecords));
  });
}

static void addTransaction(
    std::vector<std::shared_ptr<ObservableTransaction>> &records,
    ObservableTransaction::Factory &factory, Account::Observer *observer,
    void (Account::Observer::*notify)(ObservableTransaction &),
    const Transaction &transaction,
    const std::vector<std::shared_ptr<ObservableTransaction>> &creditRecords,
    const std::vector<std::shared_ptr<ObservableTransaction>> &debitRecords) {
  records.push_back(make(factory, observer, notify, transaction));
  notifyUpdatedBalance(observer, creditRecords, debitRecords);
}

static void addTransaction(
    std::vector<std::shared_ptr<ObservableTransaction>> &records,
    ObservableTransaction::Factory &factory, Account::Observer *observer,
    void (Account::Observer::*notify)(ObservableTransaction &),
    TransactionDeserialization &deserialization,
    const std::vector<std::shared_ptr<ObservableTransaction>> &creditRecords,
    const std::vector<std::shared_ptr<ObservableTransaction>> &debitRecords) {
  records.push_back(make(factory, observer, notify, deserialization));
  notifyUpdatedBalance(observer, creditRecords, debitRecords);
}

InMemoryAccount::InMemoryAccount(std::string name,
                                 ObservableTransaction::Factory &factory)
    : name{std::move(name)}, factory{factory} {}

void InMemoryAccount::attach(Observer *a) { observer = a; }

void InMemoryAccount::credit(const Transaction &transaction) {
  addTransaction(creditRecords, factory, observer,
                 &Observer::notifyThatCreditHasBeenAdded, transaction,
                 creditRecords, debitRecords);
}

void InMemoryAccount::debit(const Transaction &transaction) {
  addTransaction(debitRecords, factory, observer,
                 &Observer::notifyThatDebitHasBeenAdded, transaction,
                 creditRecords, debitRecords);
}

void InMemoryAccount::notifyThatCreditIsReady(
    TransactionDeserialization &deserialization) {
  addTransaction(creditRecords, factory, observer,
                 &Observer::notifyThatCreditHasBeenAdded, deserialization,
                 creditRecords, debitRecords);
}

void InMemoryAccount::notifyThatDebitIsReady(
    TransactionDeserialization &deserialization) {
  addTransaction(debitRecords, factory, observer,
                 &Observer::notifyThatDebitHasBeenAdded, deserialization,
                 creditRecords, debitRecords);
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
  serialization.save(name, collect(creditRecords), collect(debitRecords));
}

void InMemoryAccount::load(AccountDeserialization &deserialization) {
  deserialization.load(*this);
}

static void removeTransaction(
    std::vector<std::shared_ptr<ObservableTransaction>> &records,
    Account::Observer *observer, const Transaction &transaction,
    const std::vector<std::shared_ptr<ObservableTransaction>> &creditRecords,
    const std::vector<std::shared_ptr<ObservableTransaction>> &debitRecords) {
  if (const auto found = find_if(
          records.begin(), records.end(),
          [&transaction](const std::shared_ptr<ObservableTransaction> &record) {
            return record->removes(transaction);
          });
      found != records.end()) {
    records.erase(found);
    notifyUpdatedBalance(observer, creditRecords, debitRecords);
  }
}

void InMemoryAccount::removeDebit(const Transaction &transaction) {
  removeTransaction(debitRecords, observer, transaction, creditRecords,
                    debitRecords);
}

void InMemoryAccount::removeCredit(const Transaction &transaction) {
  removeTransaction(creditRecords, observer, transaction, creditRecords,
                    debitRecords);
}

void InMemoryAccount::rename(std::string_view to) { name = to; }

void InMemoryAccount::verifyCredit(const Transaction &transaction) {
  verify(transaction, creditRecords);
}

void InMemoryAccount::verifyDebit(const Transaction &transaction) {
  verify(transaction, debitRecords);
}

static void
addReduction(ObservableTransaction::Factory &factory,
             Account::Observer *observer,
             void (Account::Observer::*notify)(ObservableTransaction &),
             USD amount, const Date &date,
             std::vector<std::shared_ptr<ObservableTransaction>> &records) {
  auto transactionRecord{
      make(factory, observer, notify, {amount, "reduction", date})};
  transactionRecord->verify();
  records.push_back(std::move(transactionRecord));
}

static void
clear(std::vector<std::shared_ptr<ObservableTransaction>> &records) {
  for (const auto &record : records)
    record->remove();
  records.clear();
}

void InMemoryAccount::reduce(const Date &date) {
  const auto balance{budget::balance(creditRecords, debitRecords)};
  clear(debitRecords);
  clear(creditRecords);
  if (balance.cents < 0)
    addReduction(factory, observer, &Observer::notifyThatDebitHasBeenAdded,
                 -balance, date, debitRecords);
  else if (balance.cents > 0)
    addReduction(factory, observer, &Observer::notifyThatCreditHasBeenAdded,
                 balance, date, creditRecords);
}

auto InMemoryAccount::balance() -> USD {
  return budget::balance(creditRecords, debitRecords);
}

void InMemoryAccount::remove() {
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatWillBeRemoved();
  });
}

auto InMemoryAccount::Factory::make(std::string_view name_)
    -> std::shared_ptr<Account> {
  return std::make_shared<InMemoryAccount>(std::string{name_},
                                           observableTransactionFactory);
}

void ObservableTransactionInMemory::attach(Observer *a) { observer = a; }

void ObservableTransactionInMemory::initialize(const Transaction &transaction) {
  verifiableTransaction.transaction = transaction;
  callIfObserverExists(observer, [&transaction](Observer *observer_) {
    observer_->notifyThatIs(transaction);
  });
}

static void verify(VerifiableTransaction &verifiableTransaction,
                   ObservableTransaction::Observer *observer) {
  verifiableTransaction.verified = true;
  callIfObserverExists(observer,
                       [](ObservableTransaction::Observer *observer_) {
                         observer_->notifyThatIsVerified();
                       });
}

void ObservableTransactionInMemory::verify() {
  budget::verify(verifiableTransaction, observer);
}

auto ObservableTransactionInMemory::verifies(const Transaction &transaction)
    -> bool {
  if (!verifiableTransaction.verified &&
      verifiableTransaction.transaction == transaction) {
    budget::verify(verifiableTransaction, observer);
    return true;
  }
  return false;
}

auto ObservableTransactionInMemory::removes(const Transaction &transaction)
    -> bool {
  if (verifiableTransaction.transaction == transaction) {
    callIfObserverExists(observer, [](Observer *observer_) {
      observer_->notifyThatWillBeRemoved();
    });
    return true;
  }
  return false;
}

void ObservableTransactionInMemory::save(
    TransactionSerialization &serialization) {
  serialization.save(verifiableTransaction);
}

void ObservableTransactionInMemory::load(
    TransactionDeserialization &deserialization) {
  deserialization.load(*this);
}

auto ObservableTransactionInMemory::amount() -> USD {
  return verifiableTransaction.transaction.amount;
}

void ObservableTransactionInMemory::ready(
    const VerifiableTransaction &loadedVerifiableTransaction) {
  verifiableTransaction = loadedVerifiableTransaction;
  callIfObserverExists(
      observer, [&loadedVerifiableTransaction](Observer *observer_) {
        observer_->notifyThatIs(loadedVerifiableTransaction.transaction);
        if (loadedVerifiableTransaction.verified)
          observer_->notifyThatIsVerified();
      });
}

void ObservableTransactionInMemory::remove() {
  callIfObserverExists(observer, [&](Observer *observer_) {
    observer_->notifyThatWillBeRemoved();
  });
}

auto ObservableTransactionInMemory::Factory::make()
    -> std::shared_ptr<ObservableTransaction> {
  return std::make_shared<ObservableTransactionInMemory>();
}

InMemoryAccount::Factory::Factory(
    ObservableTransaction::Factory &observableTransactionFactory)
    : observableTransactionFactory{observableTransactionFactory} {}
} // namespace sbash64::budget
