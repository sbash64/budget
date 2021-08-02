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
  verifiableTransaction.transaction = transaction;
  callIfObserverExists(observer, [&transaction](Observer *observer_) {
    observer_->notifyThatIs(transaction);
  });
}

auto ObservableTransactionInMemory::verifies(const Transaction &transaction)
    -> bool {
  if (!verifiableTransaction.verified &&
      verifiableTransaction.transaction == transaction) {
    verifiableTransaction.verified = true;
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

auto ObservableTransactionInMemory::removes(const Transaction &transaction)
    -> bool {
  if (verifiableTransaction.transaction == transaction) {
    budget::remove(observer);
    return true;
  }
  return false;
}

void ObservableTransactionInMemory::remove() { budget::remove(observer); }

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

auto ObservableTransactionInMemory::Factory::make()
    -> std::shared_ptr<ObservableTransaction> {
  return std::make_shared<ObservableTransactionInMemory>();
}
} // namespace sbash64::budget
