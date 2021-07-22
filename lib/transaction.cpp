#include "transaction.hpp"

namespace sbash64::budget {
static void callIfObserverExists(
    ObservableTransaction::Observer *observer,
    const std::function<void(ObservableTransaction::Observer *)> &f) {
  if (observer != nullptr)
    f(observer);
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
} // namespace sbash64::budget