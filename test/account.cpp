#include "account.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"

#include <sbash64/budget/account.hpp>
#include <sbash64/budget/transaction.hpp>

#include <sbash64/testcpplite/testcpplite.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace sbash64::budget::account {
namespace {
class ObservableTransactionStub : public ObservableTransaction {
public:
  void ready(const ArchivableVerifiableTransaction &) override {}

  void attach(Observer *) override {}

  void initialize(const Transaction &t) override {
    initializedTransaction_ = t;
  }

  auto initializedTransaction() -> Transaction {
    return initializedTransaction_;
  }

  auto verifies(const Transaction &) -> bool override { return {}; }

  auto removes(const Transaction &t) -> bool override {
    removesed_ = true;
    removesTransaction_ = &t;
    return removes_;
  }

  [[nodiscard]] auto removesed() const -> bool { return removesed_; }

  auto removesTransaction() -> const Transaction * {
    return removesTransaction_;
  }

  void setRemoves() { removes_ = true; }

  void save(TransactionSerialization &) override {}

  void load(TransactionDeserialization &) override {}

  auto amount() -> USD override { return amount_; }

  void setAmount(USD x) { amount_ = x; }

  void remove() override {}

  void archive() override { wasArchived_ = true; }

  auto archived() -> bool override { return archived_; }

  void setVerified() { verified_ = true; }

  auto verified() -> bool override { return verified_; }

  [[nodiscard]] auto wasArchived() const -> bool { return wasArchived_; }

  void setArchived() { archived_ = true; }

private:
  Transaction initializedTransaction_;
  const Transaction *removesTransaction_{};
  USD amount_;
  bool removes_{};
  bool removesed_{};
  bool verified_{};
  bool wasArchived_{};
  bool archived_{};
};

class ObservableTransactionFactoryStub : public ObservableTransaction::Factory {
public:
  void add(std::shared_ptr<ObservableTransaction> another) {
    transactions.push_back(std::move(another));
  }

  auto make() -> std::shared_ptr<ObservableTransaction> override {
    if (transactions.empty())
      return std::make_shared<ObservableTransactionStub>();
    auto next{transactions.front()};
    transactions.erase(transactions.begin());
    return next;
  }

private:
  std::vector<std::shared_ptr<ObservableTransaction>> transactions;
};

class TransactionObserverStub : public ObservableTransaction::Observer {
public:
  void notifyThatIsVerified() override { verified_ = true; }

  void notifyThatIs(const Transaction &) override {}

  [[nodiscard]] auto verified() const -> bool { return verified_; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

  void notifyThatWillBeRemoved() override { removed_ = true; }

  void notifyThatIsArchived() override {}

private:
  bool verified_{};
  bool removed_{};
};

class AccountObserverStub : public Account::Observer {
public:
  void notifyThatNameHasChanged(std::string_view) override {}

  auto allocation() -> USD { return allocation_; }

  auto balance() -> USD { return balance_; }

  void notifyThatBalanceHasChanged(USD balance) override { balance_ = balance; }

  void notifyThatAllocationHasChanged(USD usd) override { allocation_ = usd; }

  void notifyThatHasBeenAdded(ObservableTransaction &tr) override {
    newTransactionRecord_ = &tr;
  }

  auto newTransactionRecord() -> const ObservableTransaction * {
    return newTransactionRecord_;
  }

  void notifyThatWillBeRemoved() override { willBeRemoved_ = true; }

  [[nodiscard]] auto willBeRemoved() const -> bool { return willBeRemoved_; }

private:
  ObservableTransaction *newTransactionRecord_{};
  USD balance_{};
  USD allocation_{};
  bool willBeRemoved_{};
};

class TransactionDeserializationStub : public TransactionDeserialization {
public:
  void load(Observer &a) override { observer_ = &a; }

  auto observer() -> Observer * { return observer_; }

private:
  Observer *observer_{};
};
} // namespace

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<SerializableTransaction *> &expected,
                        const std::vector<SerializableTransaction *> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<SerializableTransaction *>::size_type i{0};
       i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

static void
assertSaved(testcpplite::TestResult &result, PersistentAccountStub &persistence,
            const std::vector<SerializableTransaction *> &transactions) {
  assertEqual(result, persistence.transactions(), transactions);
}

static void add(Account &account, const Transaction &t = {}) { account.add(t); }

static void testInMemoryAccount(
    const std::function<void(AccountInMemory &,
                             ObservableTransactionFactoryStub &)> &test) {
  ObservableTransactionFactoryStub factory;
  AccountInMemory account{factory};
  test(account, factory);
}

static void assertBalanceEquals(testcpplite::TestResult &result, USD actual,
                                AccountObserverStub &observer) {
  assertEqual(result, actual, observer.balance());
}

static auto
addObservableTransactionStub(ObservableTransactionFactoryStub &factory)
    -> std::shared_ptr<ObservableTransactionStub> {
  auto transaction{std::make_shared<ObservableTransactionStub>()};
  factory.add(transaction);
  return transaction;
}

static auto
addObservableTransactionInMemory(ObservableTransactionFactoryStub &factory)
    -> std::shared_ptr<ObservableTransactionInMemory> {
  auto transaction{std::make_shared<ObservableTransactionInMemory>()};
  factory.add(transaction);
  return transaction;
}

void initializesAddedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto ape{addObservableTransactionStub(factory)};
    add(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                ape->initializedTransaction());
  });
}

void notifiesObserverOfRemoval(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](AccountInMemory &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.remove();
        assertTrue(result, observer.willBeRemoved());
      });
}

void notifiesObserverOfNewCredit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto record{addObservableTransactionStub(factory)};
    add(account);
    assertEqual(result, record.get(), observer.newTransactionRecord());
  });
}

void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    addObservableTransactionStub(factory)->setAmount(3_cents);
    add(account);
    assertBalanceEquals(result, 3_cents, observer);
    addObservableTransactionStub(factory)->setAmount(11_cents);
    add(account);
    assertBalanceEquals(result, 3_cents + 11_cents, observer);
  });
}

void savesAllTransactionsAndAccountName(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto john{addObservableTransactionStub(factory)};
    add(account);
    const auto andy{addObservableTransactionStub(factory)};
    add(account);
    account.increaseAllocationBy(1_cents);
    PersistentAccountStub persistence;
    account.save(persistence);
    assertSaved(result, persistence, {john.get(), andy.get()});
    assertEqual(result, 1_cents, persistence.allocation());
  });
}

void attemptsToRemoveEachCreditUntilFound(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    TransactionDeserializationStub deserialization;
    const auto mike{addObservableTransactionStub(factory)};
    account.notifyThatIsReady(deserialization);
    const auto andy{addObservableTransactionStub(factory)};
    account.notifyThatIsReady(deserialization);
    const auto joe{addObservableTransactionStub(factory)};
    account.notifyThatIsReady(deserialization);
    const auto bob{addObservableTransactionStub(factory)};
    account.notifyThatIsReady(deserialization);
    andy->setRemoves();
    Transaction transaction;
    account.remove(transaction);
    assertEqual(result, &transaction, mike->removesTransaction());
    assertEqual(result, &transaction, andy->removesTransaction());
    assertFalse(result, joe->removesed());
    assertFalse(result, bob->removesed());
  });
}

void savesLoadedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    TransactionDeserializationStub deserialization;
    const auto mike{addObservableTransactionStub(factory)};
    account.notifyThatIsReady(deserialization);
    const auto joe{addObservableTransactionStub(factory)};
    account.notifyThatIsReady(deserialization);
    PersistentAccountStub persistence;
    account.save(persistence);
    assertSaved(result, persistence, {mike.get(), joe.get()});
  });
}

void savesRemainingTransactionsAfterRemovingSome(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto ape{addObservableTransactionStub(factory)};
    add(account);
    const auto orangutan{addObservableTransactionStub(factory)};
    add(account);
    orangutan->setRemoves();
    account.remove({});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertSaved(result, persistence, {ape.get()});
  });
}

void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto ape{addObservableTransactionStub(factory)};
    const auto orangutan{addObservableTransactionStub(factory)};
    const auto chimp{addObservableTransactionStub(factory)};
    add(account);
    add(account);
    add(account);
    ape->setAmount(1_cents);
    orangutan->setAmount(2_cents);
    chimp->setAmount(3_cents);
    orangutan->setRemoves();
    account.remove({});
    assertBalanceEquals(result, 1_cents + 3_cents, observer);
  });
}

void observesDeserialization(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](AccountInMemory &account, ObservableTransactionFactoryStub &) {
        PersistentAccountStub persistence;
        account.load(persistence);
        assertEqual(result,
                    static_cast<AccountDeserialization::Observer *>(&account),
                    persistence.observer());
      });
}

void hasTransactionsObserveDeserialization(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{addObservableTransactionStub(factory)};
    TransactionDeserializationStub abel;
    account.notifyThatIsReady(abel);
    assertEqual(result, mike.get(), abel.observer());
  });
}

void notifiesObserverThatDuplicateTransactionsAreVerified(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla1{addObservableTransactionInMemory(factory)};
    TransactionObserverStub gorilla1Observer;
    gorilla1->attach(&gorilla1Observer);
    add(account,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto gorilla2{addObservableTransactionInMemory(factory)};
    TransactionObserverStub gorilla2Observer;
    gorilla2->attach(&gorilla2Observer);
    add(account,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verify(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verify(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertTrue(result, gorilla1Observer.verified());
    assertTrue(result, gorilla2Observer.verified());
  });
}

void notifiesObserverOfVerifiedTransaction(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{addObservableTransactionInMemory(factory)};
    TransactionObserverStub observer;
    record->attach(&observer);
    add(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verify(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void notifiesObserverOfRemovedTransaction(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{addObservableTransactionInMemory(factory)};
    TransactionObserverStub observer;
    record->attach(&observer);
    add(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.remove(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}

void notifiesUpdatedBalanceAfterArchivingVerified(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto orangutan{addObservableTransactionStub(factory)};
    const auto gorilla{addObservableTransactionStub(factory)};
    const auto chimp{addObservableTransactionStub(factory)};
    add(account);
    add(account);
    add(account);
    orangutan->setAmount(1_cents);
    gorilla->setAmount(2_cents);
    chimp->setAmount(3_cents);
    gorilla->setArchived();
    account.increaseAllocationByResolvingVerifiedTransactions();
    assertEqual(result, 1_cents + 3_cents, account.balance());
    assertEqual(result, 1_cents + 3_cents, observer.balance());
  });
}

void archivesVerifiedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto orangutan{addObservableTransactionStub(factory)};
    const auto gorilla{addObservableTransactionStub(factory)};
    const auto chimp{addObservableTransactionStub(factory)};
    add(account);
    add(account);
    add(account);
    gorilla->setVerified();
    account.increaseAllocationByResolvingVerifiedTransactions();
    assertFalse(result, orangutan->wasArchived());
    assertFalse(result, chimp->wasArchived());
    assertTrue(result, gorilla->wasArchived());
  });
}

void returnsBalance(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto orangutan{addObservableTransactionStub(factory)};
    const auto gorilla{addObservableTransactionStub(factory)};
    add(account);
    add(account);
    orangutan->setAmount(1_cents);
    gorilla->setAmount(2_cents);
    assertEqual(result, 1_cents + 2_cents, account.balance());
  });
}

void notifiesObserverOfUpdatedBalanceOnClear(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto orangutan{addObservableTransactionStub(factory)};
    orangutan->setAmount(1_cents);
    add(account);
    account.increaseAllocationBy(2_cents);
    account.clear();
    assertEqual(result, 0_cents, observer.balance());
    assertEqual(result, 0_cents, observer.allocation());
  });
}

void increasesAllocationByAmountArchived(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto orangutan{addObservableTransactionInMemory(factory)};
    const auto gorilla{addObservableTransactionInMemory(factory)};
    const auto chimp{addObservableTransactionInMemory(factory)};
    add(account, Transaction{2_cents, "gorilla", Date{2020, Month::June, 2}});
    add(account, Transaction{3_cents, "chimp", Date{2020, Month::June, 2}});
    add(account, Transaction{4_cents, "orangutan", Date{2020, Month::June, 2}});
    account.verify(Transaction{2_cents, "gorilla", Date{2020, Month::June, 2}});
    account.verify(Transaction{3_cents, "chimp", Date{2020, Month::June, 2}});
    account.increaseAllocationByResolvingVerifiedTransactions();
    assertEqual(result, 2_cents + 3_cents, observer.allocation());
    assertEqual(result, 4_cents, observer.balance());
    account.verify(
        Transaction{4_cents, "orangutan", Date{2020, Month::June, 2}});
    account.increaseAllocationByResolvingVerifiedTransactions();
    assertEqual(result, 2_cents + 3_cents + 4_cents, observer.allocation());
    assertEqual(result, 0_cents, observer.balance());
  });
}

void doesNotRemoveArchivedTransaction(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto orangutan1{addObservableTransactionInMemory(factory)};
    add(account, Transaction{4_cents, "orangutan", Date{2020, Month::June, 2}});
    account.verify(
        Transaction{4_cents, "orangutan", Date{2020, Month::June, 2}});
    account.increaseAllocationByResolvingVerifiedTransactions();
    assertEqual(result, 4_cents, observer.allocation());
    assertEqual(result, 0_cents, observer.balance());
    const auto orangutan2{addObservableTransactionInMemory(factory)};
    add(account, Transaction{4_cents, "orangutan", Date{2020, Month::June, 2}});
    account.remove(
        Transaction{4_cents, "orangutan", Date{2020, Month::June, 2}});
    assertEqual(result, 0_cents, observer.balance());
  });
}

void decreasesAllocationByAmountArchived(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](AccountInMemory &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto orangutan{addObservableTransactionInMemory(factory)};
    const auto gorilla{addObservableTransactionInMemory(factory)};
    const auto chimp{addObservableTransactionInMemory(factory)};
    add(account, Transaction{2_cents, "gorilla", Date{2020, Month::June, 2}});
    add(account, Transaction{3_cents, "chimp", Date{2020, Month::June, 2}});
    add(account, Transaction{4_cents, "orangutan", Date{2020, Month::June, 2}});
    account.verify(Transaction{2_cents, "gorilla", Date{2020, Month::June, 2}});
    account.verify(Transaction{3_cents, "chimp", Date{2020, Month::June, 2}});
    account.decreaseAllocationByResolvingVerifiedTransactions();
    assertEqual(result, -2_cents - 3_cents, observer.allocation());
    assertEqual(result, 4_cents, observer.balance());
  });
}

void notifiesObserverOfIncreasedAllocation(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](AccountInMemory &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.increaseAllocationBy(1_cents);
        assertEqual(result, 1_cents, account.allocated());
        assertEqual(result, 1_cents, observer.allocation());
      });
}

void notifiesObserverOfDecreasedAllocation(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](AccountInMemory &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.decreaseAllocationBy(1_cents);
        assertEqual(result, -1_cents, account.allocated());
        assertEqual(result, -1_cents, observer.allocation());
      });
}

void notifiesObserverOfLoadedAllocation(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](AccountInMemory &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.notifyThatAllocatedIsReady(1_cents);
        assertEqual(result, 1_cents, account.allocated());
        assertEqual(result, 1_cents, observer.allocation());
      });
}
} // namespace sbash64::budget::account
