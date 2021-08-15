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

static auto
balance(USD funds,
        const std::vector<std::shared_ptr<ObservableTransaction>> &credits,
        const std::vector<std::shared_ptr<ObservableTransaction>> &debits)
    -> USD {
  return funds + balance(credits) - balance(debits);
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
    Account::Observer *observer, USD funds,
    const std::vector<std::shared_ptr<ObservableTransaction>> &creditRecords,
    const std::vector<std::shared_ptr<ObservableTransaction>> &debitRecords) {
  callIfObserverExists(observer, [&](InMemoryAccount::Observer *observer_) {
    observer_->notifyThatBalanceHasChanged(
        budget::balance(funds, creditRecords, debitRecords));
  });
}

static void addTransaction(
    std::vector<std::shared_ptr<ObservableTransaction>> &records,
    ObservableTransaction::Factory &factory, Account::Observer *observer,
    void (Account::Observer::*notify)(ObservableTransaction &),
    const Transaction &transaction, USD funds,
    const std::vector<std::shared_ptr<ObservableTransaction>> &creditRecords,
    const std::vector<std::shared_ptr<ObservableTransaction>> &debitRecords) {
  records.push_back(make(factory, observer, notify, transaction));
  notifyUpdatedBalance(observer, funds, creditRecords, debitRecords);
}

static void addTransaction(
    std::vector<std::shared_ptr<ObservableTransaction>> &records,
    ObservableTransaction::Factory &factory, Account::Observer *observer,
    void (Account::Observer::*notify)(ObservableTransaction &),
    TransactionDeserialization &deserialization, USD funds,
    const std::vector<std::shared_ptr<ObservableTransaction>> &creditRecords,
    const std::vector<std::shared_ptr<ObservableTransaction>> &debitRecords) {
  records.push_back(make(factory, observer, notify, deserialization));
  notifyUpdatedBalance(observer, funds, creditRecords, debitRecords);
}

static void removeTransaction(
    std::vector<std::shared_ptr<ObservableTransaction>> &records,
    Account::Observer *observer, const Transaction &transaction, USD funds,
    const std::vector<std::shared_ptr<ObservableTransaction>> &creditRecords,
    const std::vector<std::shared_ptr<ObservableTransaction>> &debitRecords) {
  if (const auto found = find_if(
          records.begin(), records.end(),
          [&transaction](const std::shared_ptr<ObservableTransaction> &record) {
            return record->removes(transaction);
          });
      found != records.end()) {
    records.erase(found);
    notifyUpdatedBalance(observer, funds, creditRecords, debitRecords);
  }
}

InMemoryAccount::InMemoryAccount(std::string name,
                                 ObservableTransaction::Factory &factory)
    : name{std::move(name)}, factory{factory} {}

void InMemoryAccount::attach(Observer *a) { observer = a; }

void InMemoryAccount::credit(const Transaction &transaction) {
  addTransaction(creditRecords, factory, observer,
                 &Observer::notifyThatCreditHasBeenAdded, transaction, funds,
                 creditRecords, debitRecords);
}

void InMemoryExpenseAccount::debit(const Transaction &transaction) {
  addTransaction(debitRecords, factory, observer,
                 &Observer::notifyThatDebitHasBeenAdded, transaction, funds,
                 creditRecords, debitRecords);
}

void InMemoryAccount::removeCredit(const Transaction &transaction) {
  removeTransaction(creditRecords, observer, transaction, funds, creditRecords,
                    debitRecords);
}

void InMemoryExpenseAccount::removeDebit(const Transaction &transaction) {
  removeTransaction(debitRecords, observer, transaction, funds, creditRecords,
                    debitRecords);
}

void InMemoryAccount::verifyCredit(const Transaction &transaction) {
  verify(transaction, creditRecords);
}

void InMemoryExpenseAccount::verifyDebit(const Transaction &transaction) {
  verify(transaction, debitRecords);
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

void InMemoryAccount::rename(std::string_view to) { name = to; }

void InMemoryAccount::save(AccountSerialization &serialization) {
  serialization.save(name, funds, collect(creditRecords),
                     collect(debitRecords));
}

void InMemoryAccount::load(AccountDeserialization &deserialization) {
  deserialization.load(*this);
}

void InMemoryAccount::notifyThatCreditIsReady(
    TransactionDeserialization &deserialization) {
  addTransaction(creditRecords, factory, observer,
                 &Observer::notifyThatCreditHasBeenAdded, deserialization,
                 funds, creditRecords, debitRecords);
}

void InMemoryAccount::notifyThatDebitIsReady(
    TransactionDeserialization &deserialization) {
  addTransaction(debitRecords, factory, observer,
                 &Observer::notifyThatDebitHasBeenAdded, deserialization, funds,
                 creditRecords, debitRecords);
}

static void notifyUpdatedFunds(Account::Observer *observer, USD funds) {
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatFundsHaveChanged(funds);
  });
}

void InMemoryAccount::notifyThatFundsAreReady(USD usd) {
  funds = usd;
  notifyUpdatedFunds(observer, funds);
  notifyUpdatedBalance(observer, funds, creditRecords, debitRecords);
}

static void
clear(std::vector<std::shared_ptr<ObservableTransaction>> &records) {
  for (const auto &record : records)
    record->remove();
  records.clear();
}

void InMemoryAccount::reduce() {
  funds = budget::balance(funds, creditRecords, debitRecords);
  notifyUpdatedFunds(observer, funds);
  budget::clear(debitRecords);
  budget::clear(creditRecords);
}

auto InMemoryAccount::balance() -> USD {
  return budget::balance(funds, creditRecords, debitRecords);
}

void InMemoryAccount::withdraw(USD usd) {
  funds -= usd;
  notifyUpdatedFunds(observer, funds);
  notifyUpdatedBalance(observer, funds, creditRecords, debitRecords);
}

void InMemoryAccount::deposit(USD usd) {
  funds += usd;
  notifyUpdatedFunds(observer, funds);
  notifyUpdatedBalance(observer, funds, creditRecords, debitRecords);
}

void InMemoryAccount::remove() {
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatWillBeRemoved();
  });
}

void InMemoryAccount::clear() {
  funds = {};
  notifyUpdatedFunds(observer, funds);
  budget::clear(creditRecords);
  budget::clear(debitRecords);
  notifyUpdatedBalance(observer, funds, creditRecords, debitRecords);
}

auto InMemoryAccount::Factory::make(std::string_view name_)
    -> std::shared_ptr<ExpenseAccount> {
  return std::make_shared<InMemoryExpenseAccount>(std::string{name_},
                                                  transactionFactory);
}

InMemoryAccount::Factory::Factory(
    ObservableTransaction::Factory &transactionFactory)
    : transactionFactory{transactionFactory} {}
} // namespace sbash64::budget
