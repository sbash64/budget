#include "transaction.hpp"

#include <functional>

namespace sbash64::budget {
void ObservableTransactionInMemory::attach(Observer &a) {
  observers.push_back(std::ref(a));
}

void ObservableTransactionInMemory::initialize(const Transaction &transaction) {
  static_cast<Transaction &>(archivableVerifiableTransaction) = transaction;
  for (auto observer : observers)
    observer.get().notifyThatIs(transaction);
}

auto ObservableTransactionInMemory::verifies(const Transaction &match) -> bool {
  if (!archivableVerifiableTransaction.verified &&
      static_cast<Transaction &>(archivableVerifiableTransaction) == match) {
    archivableVerifiableTransaction.verified = true;
    for (auto observer : observers)
      observer.get().notifyThatIsVerified();
    return true;
  }
  return false;
}

static void remove(
    const std::vector<std::reference_wrapper<ObservableTransaction::Observer>>
        &observers) {
  for (auto observer : observers)
    observer.get().notifyThatWillBeRemoved();
}

auto ObservableTransactionInMemory::removes(const Transaction &match) -> bool {
  if (static_cast<Transaction &>(archivableVerifiableTransaction) == match) {
    budget::remove(observers);
    return true;
  }
  return false;
}

void ObservableTransactionInMemory::archive() {
  if (!archivableVerifiableTransaction.archived) {
    archivableVerifiableTransaction.archived = true;
    for (auto observer : observers)
      observer.get().notifyThatIsArchived();
  }
}

auto ObservableTransactionInMemory::verified() -> bool {
  return archivableVerifiableTransaction.verified;
}

void ObservableTransactionInMemory::remove() { budget::remove(observers); }

void ObservableTransactionInMemory::save(
    TransactionSerialization &serialization) {
  serialization.save(archivableVerifiableTransaction);
}

auto ObservableTransactionInMemory::amount() -> USD {
  return archivableVerifiableTransaction.amount;
}

auto ObservableTransactionInMemory::Factory::make()
    -> std::shared_ptr<ObservableTransaction> {
  return std::make_shared<ObservableTransactionInMemory>();
}
} // namespace sbash64::budget
