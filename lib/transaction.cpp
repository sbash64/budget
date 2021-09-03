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
  archivableVerifiableTransaction.transaction = transaction;
  callIfObserverExists(observer, [&transaction](Observer *observer_) {
    observer_->notifyThatIs(transaction);
  });
}

auto ObservableTransactionInMemory::verifies(const Transaction &match) -> bool {
  if (!archivableVerifiableTransaction.verified &&
      archivableVerifiableTransaction.transaction == match) {
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
  if (archivableVerifiableTransaction.transaction == match) {
    budget::remove(observer);
    return true;
  }
  return false;
}

void ObservableTransactionInMemory::archive() {
  archivableVerifiableTransaction.archived = true;
  callIfObserverExists(observer,
                       [](ObservableTransaction::Observer *observer_) {
                         observer_->notifyThatIsArchived();
                       });
}

auto ObservableTransactionInMemory::archived() -> bool {
  return archivableVerifiableTransaction.archived;
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
  return archivableVerifiableTransaction.transaction.amount;
}

void ObservableTransactionInMemory::ready(
    const VerifiableTransaction &loadedVerifiableTransaction) {
  archivableVerifiableTransaction.transaction =
      loadedVerifiableTransaction.transaction;
  archivableVerifiableTransaction.verified =
      loadedVerifiableTransaction.verified;
  callIfObserverExists(
      observer, [&loadedVerifiableTransaction](Observer *observer_) {
        observer_->notifyThatIs(loadedVerifiableTransaction.transaction);
        if (loadedVerifiableTransaction.verified)
          observer_->notifyThatIsVerified();
      });
}

auto ObservableTransactionInMemory::Factory::make()
    -> std::shared_ptr<ObservableTransaction> {
  return std::make_shared<ObservableTransactionInMemory>();
}
} // namespace sbash64::budget
