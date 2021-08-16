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

static void notifyUpdatedBalance(Account &account, Account::Observer *observer,
                                 USD funds) {
  callIfObserverExists(observer, [&](InMemoryAccount::Observer *observer_) {
    observer_->notifyThatBalanceHasChanged(funds + account.balance());
  });
}

static void
addTransaction(std::vector<std::shared_ptr<ObservableTransaction>> &records,
               ObservableTransaction::Factory &factory, Account &account,
               Account::Observer *observer,
               void (Account::Observer::*notify)(ObservableTransaction &),
               const Transaction &transaction, USD funds) {
  records.push_back(make(factory, observer, notify, transaction));
  notifyUpdatedBalance(account, observer, funds);
}

static void
addTransaction(std::vector<std::shared_ptr<ObservableTransaction>> &records,
               ObservableTransaction::Factory &factory, Account &account,
               Account::Observer *observer,
               void (Account::Observer::*notify)(ObservableTransaction &),
               TransactionDeserialization &deserialization, USD funds) {
  records.push_back(make(factory, observer, notify, deserialization));
  notifyUpdatedBalance(account, observer, funds);
}

static void
removeTransaction(std::vector<std::shared_ptr<ObservableTransaction>> &records,
                  Account &account, Account::Observer *observer,
                  const Transaction &transaction, USD funds) {
  if (const auto found = find_if(
          records.begin(), records.end(),
          [&transaction](const std::shared_ptr<ObservableTransaction> &record) {
            return record->removes(transaction);
          });
      found != records.end()) {
    records.erase(found);
    notifyUpdatedBalance(account, observer, funds);
  }
}

InMemoryAccount::InMemoryAccount(std::string name,
                                 ObservableTransaction::Factory &factory)
    : name{std::move(name)}, factory{factory} {}

void InMemoryAccount::attach(Observer *a) { observer = a; }

void InMemoryIncomeAccount::add(const Transaction &transaction) {
  addTransaction(creditRecords, factory, *this, observer,
                 &Observer::notifyThatCreditHasBeenAdded, transaction, funds);
}

void InMemoryExpenseAccount::add(const Transaction &transaction) {
  addTransaction(debitRecords, factory, *this, observer,
                 &Observer::notifyThatDebitHasBeenAdded, transaction, funds);
}

void InMemoryIncomeAccount::remove(const Transaction &transaction) {
  removeTransaction(creditRecords, *this, observer, transaction, funds);
}

void InMemoryExpenseAccount::remove(const Transaction &transaction) {
  removeTransaction(debitRecords, *this, observer, transaction, funds);
}

void InMemoryIncomeAccount::verify(const Transaction &transaction) {
  budget::verify(transaction, creditRecords);
}

void InMemoryExpenseAccount::verify(const Transaction &transaction) {
  budget::verify(transaction, debitRecords);
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
  addTransaction(creditRecords, factory, *this, observer,
                 &Observer::notifyThatCreditHasBeenAdded, deserialization,
                 funds);
}

void InMemoryAccount::notifyThatDebitIsReady(
    TransactionDeserialization &deserialization) {
  addTransaction(debitRecords, factory, *this, observer,
                 &Observer::notifyThatDebitHasBeenAdded, deserialization,
                 funds);
}

static void notifyUpdatedFunds(Account::Observer *observer, USD funds) {
  callIfObserverExists(observer, [&](Account::Observer *observer_) {
    observer_->notifyThatFundsHaveChanged(funds);
  });
}

void InMemoryAccount::notifyThatFundsAreReady(USD usd) {
  funds = usd;
  notifyUpdatedFunds(observer, funds);
  notifyUpdatedBalance(*this, observer, funds);
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

auto InMemoryExpenseAccount::balance() -> USD {
  return funds - budget::balance(debitRecords);
}

auto InMemoryIncomeAccount::balance() -> USD {
  return funds + budget::balance(creditRecords);
}

void InMemoryAccount::withdraw(USD usd) {
  funds -= usd;
  notifyUpdatedFunds(observer, funds);
  notifyUpdatedBalance(*this, observer, funds);
}

void InMemoryAccount::deposit(USD usd) {
  funds += usd;
  notifyUpdatedFunds(observer, funds);
  notifyUpdatedBalance(*this, observer, funds);
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
  notifyUpdatedBalance(*this, observer, funds);
}

auto InMemoryExpenseAccount::Factory::make(std::string_view name_)
    -> std::shared_ptr<ExpenseAccount> {
  return std::make_shared<InMemoryExpenseAccount>(std::string{name_},
                                                  transactionFactory);
}

InMemoryExpenseAccount::Factory::Factory(
    ObservableTransaction::Factory &transactionFactory)
    : transactionFactory{transactionFactory} {}
} // namespace sbash64::budget
