#include "account.hpp"
#include <numeric>

namespace sbash64::budget {
static void callIfObserverExists(
    InMemoryAccount::Observer *observer,
    const std::function<void(InMemoryAccount::Observer *)> &f) {
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

InMemoryAccount::InMemoryAccount(std::string name,
                                 ObservableTransaction::Factory &factory)
    : name{std::move(name)}, factory{factory} {}

void InMemoryAccount::attach(Observer *a) { observer = a; }

void InMemoryAccount::credit(const Transaction &transaction) {
  creditRecords.push_back(make(
      factory, observer, &Observer::notifyThatCreditHasBeenAdded, transaction));
  notifyUpdatedBalance(observer, creditRecords, debitRecords);
}

void InMemoryAccount::debit(const Transaction &transaction) {
  debitRecords.push_back(make(
      factory, observer, &Observer::notifyThatDebitHasBeenAdded, transaction));
  notifyUpdatedBalance(observer, creditRecords, debitRecords);
}

void InMemoryAccount::notifyThatCreditIsReady(
    TransactionDeserialization &deserialization) {
  creditRecords.push_back(make(factory, observer,
                               &Observer::notifyThatCreditHasBeenAdded,
                               deserialization));
  notifyUpdatedBalance(observer, creditRecords, debitRecords);
}

void InMemoryAccount::notifyThatDebitIsReady(
    TransactionDeserialization &deserialization) {
  debitRecords.push_back(make(factory, observer,
                              &Observer::notifyThatDebitHasBeenAdded,
                              deserialization));
  notifyUpdatedBalance(observer, creditRecords, debitRecords);
}

static auto
collect(const std::vector<std::shared_ptr<ObservableTransaction>> &accounts)
    -> std::vector<ObservableTransaction *> {
  std::vector<ObservableTransaction *> collected;
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

void InMemoryAccount::removeDebit(const Transaction &t) {
  for (auto it{debitRecords.begin()}; it != debitRecords.end(); ++it)
    if ((*it)->removes(t)) {
      debitRecords.erase(it);
      notifyUpdatedBalance(observer, creditRecords, debitRecords);
      return;
    }
}

void InMemoryAccount::removeCredit(const Transaction &t) {
  for (auto it{creditRecords.begin()}; it != creditRecords.end(); ++it)
    if ((*it)->removes(t)) {
      creditRecords.erase(it);
      notifyUpdatedBalance(observer, creditRecords, debitRecords);
      return;
    }
}

void InMemoryAccount::rename(std::string_view s) { name = s; }

void InMemoryAccount::verifyCredit(const Transaction &t) {
  verify(t, creditRecords);
}

void InMemoryAccount::verifyDebit(const Transaction &t) {
  verify(t, debitRecords);
}

void InMemoryAccount::reduce(const Date &date) {
  const auto balance{budget::balance(creditRecords, debitRecords)};
  for (const auto &record : debitRecords)
    record->remove();
  debitRecords.clear();
  for (const auto &record : creditRecords)
    record->remove();
  creditRecords.clear();
  if (balance.cents < 0) {
    auto transactionRecord{make(factory, observer,
                                &Observer::notifyThatDebitHasBeenAdded,
                                {-balance, "reduction", date})};
    transactionRecord->verify();
    debitRecords.push_back(std::move(transactionRecord));
  } else {
    auto transactionRecord{make(factory, observer,
                                &Observer::notifyThatCreditHasBeenAdded,
                                {balance, "reduction", date})};
    transactionRecord->verify();
    creditRecords.push_back(std::move(transactionRecord));
  }
}

auto InMemoryAccount::balance() -> USD {
  return budget::balance(creditRecords, debitRecords);
}

auto InMemoryAccount::Factory::make(std::string_view name_,
                                    ObservableTransaction::Factory &factory_)
    -> std::shared_ptr<Account> {
  return std::make_shared<InMemoryAccount>(std::string{name_}, factory_);
}
void TransactionRecordInMemory::attach(Observer *a) { observer = a; }

void TransactionRecordInMemory::initialize(const Transaction &t) {
  verifiableTransaction.transaction = t;
  if (observer != nullptr)
    observer->notifyThatIs(t);
}

static void verify(VerifiableTransaction &vt,
                   ObservableTransaction::Observer *observer) {
  if (observer != nullptr)
    observer->notifyThatIsVerified();
  vt.verified = true;
}

void TransactionRecordInMemory::verify() {
  budget::verify(verifiableTransaction, observer);
}

auto TransactionRecordInMemory::verifies(const Transaction &t) -> bool {
  if (!verifiableTransaction.verified &&
      verifiableTransaction.transaction == t) {
    budget::verify(verifiableTransaction, observer);
    return true;
  }
  return false;
}

auto TransactionRecordInMemory::removes(const Transaction &t) -> bool {
  if (verifiableTransaction.transaction == t) {
    if (observer != nullptr)
      observer->notifyThatWillBeRemoved();
    return true;
  }
  return false;
}

void TransactionRecordInMemory::save(TransactionSerialization &serialization) {
  serialization.save(verifiableTransaction);
}

void TransactionRecordInMemory::load(
    TransactionDeserialization &deserialization) {
  deserialization.load(*this);
}

auto TransactionRecordInMemory::amount() -> USD {
  return verifiableTransaction.transaction.amount;
}

void TransactionRecordInMemory::ready(const VerifiableTransaction &vt) {
  verifiableTransaction = vt;
  if (observer != nullptr) {
    observer->notifyThatIs(vt.transaction);
    if (vt.verified)
      observer->notifyThatIsVerified();
  }
}

void TransactionRecordInMemory::remove() {
  if (observer != nullptr)
    observer->notifyThatWillBeRemoved();
}
} // namespace sbash64::budget
