#include "transaction.hpp"

#include <functional>

namespace sbash64::budget {
static void callIfObserverExists(
    ObservableTransaction::Observer *observer,
    const std::function<void(ObservableTransaction::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
}

void ObservableTransactionInMemory::attach(Observer *a) { observer = a; }

void ObservableTransactionInMemory::initialize(const Transaction &transaction) {
  static_cast<Transaction &>(archivableVerifiableTransaction) = transaction;
  callIfObserverExists(observer, [&transaction](Observer *observer_) {
    observer_->notifyThatIs(transaction);
  });
}

auto ObservableTransactionInMemory::verifies(const Transaction &match) -> bool {
  if (!archivableVerifiableTransaction.verified &&
      static_cast<Transaction &>(archivableVerifiableTransaction) == match) {
    archivableVerifiableTransaction.verified = true;
    callIfObserverExists(observer,
                         [](ObservableTransaction::Observer *observer_) {
                           observer_->notifyThatIsVerified();
                         });
    return true;
  }
  return false;
}

static void remove(ObservableTransaction::Observer *observer) {
  callIfObserverExists(observer,
                       [](ObservableTransaction::Observer *observer_) {
                         observer_->notifyThatWillBeRemoved();
                       });
}

auto ObservableTransactionInMemory::removes(const Transaction &match) -> bool {
  if (static_cast<Transaction &>(archivableVerifiableTransaction) == match) {
    budget::remove(observer);
    return true;
  }
  return false;
}

void ObservableTransactionInMemory::archive() {
  if (!archivableVerifiableTransaction.archived) {
    archivableVerifiableTransaction.archived = true;
    callIfObserverExists(observer,
                         [](ObservableTransaction::Observer *observer_) {
                           observer_->notifyThatIsArchived();
                         });
  }
}

auto ObservableTransactionInMemory::verified() -> bool {
  return archivableVerifiableTransaction.verified;
}

void ObservableTransactionInMemory::remove() { budget::remove(observer); }

void ObservableTransactionInMemory::save(
    TransactionSerialization &serialization) {
  serialization.save(archivableVerifiableTransaction);
}

void ObservableTransactionInMemory::load(
    TransactionDeserialization &deserialization) {
  deserialization.load(*this);
}

auto ObservableTransactionInMemory::amount() -> USD {
  return archivableVerifiableTransaction.amount;
}

void ObservableTransactionInMemory::ready(
    const ArchivableVerifiableTransaction &loadedVerifiableTransaction) {
  archivableVerifiableTransaction = loadedVerifiableTransaction;
  callIfObserverExists(observer,
                       [&loadedVerifiableTransaction](Observer *observer_) {
                         observer_->notifyThatIs(loadedVerifiableTransaction);
                         if (loadedVerifiableTransaction.verified)
                           observer_->notifyThatIsVerified();
                         if (loadedVerifiableTransaction.archived)
                           observer_->notifyThatIsArchived();
                       });
}

auto ObservableTransactionInMemory::Factory::make()
    -> std::shared_ptr<ObservableTransaction> {
  return std::make_shared<ObservableTransactionInMemory>();
}
} // namespace sbash64::budget
