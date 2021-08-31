#include "account.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"

#include <sbash64/budget/account.hpp>
#include <sbash64/budget/transaction.hpp>

#include <sbash64/testcpplite/testcpplite.hpp>

#include <functional>
#include <map>
#include <memory>
#include <utility>

namespace sbash64::budget::account {
namespace {
class ObservableTransactionStub : public ObservableTransaction {
public:
  void ready(const VerifiableTransaction &) override {}

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

  void remove() override { removed_ = true; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

private:
  Transaction initializedTransaction_;
  const Transaction *removesTransaction_{};
  USD amount_;
  bool removed_{};
  bool removes_{};
  bool removesed_{};
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

private:
  bool verified_{};
  bool removed_{};
};

class AccountObserverStub : public Account::Observer {
public:
  void notifyThatFundsHaveChanged(USD usd) override { funds_ = usd; }

  auto funds() -> USD { return funds_; }

  auto balance() -> USD { return balance_; }

  void notifyThatBalanceHasChanged(USD balance) override { balance_ = balance; }

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
  USD funds_{};
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
assertCreditsSaved(testcpplite::TestResult &result,
                   PersistentAccountStub &persistence,
                   const std::vector<SerializableTransaction *> &transactions) {
  assertEqual(result, persistence.transactions(), transactions);
}

static void credit(Account &account, const Transaction &t = {}) {
  account.add(t);
}

static void debit(Account &account, const Transaction &t = {}) {
  account.add(t);
}

namespace income {
static void testInMemoryAccount(
    const std::function<void(InMemoryAccount &,
                             ObservableTransactionFactoryStub &)> &test,
    std::string name = {}) {
  ObservableTransactionFactoryStub factory;
  InMemoryAccount account{std::move(name), factory, true};
  test(account, factory);
}
} // namespace income

namespace expense {
static void testInMemoryAccount(
    const std::function<void(InMemoryAccount &,
                             ObservableTransactionFactoryStub &)> &test,
    std::string name = {}) {
  ObservableTransactionFactoryStub factory;
  InMemoryAccount account{std::move(name), factory, false};
  test(account, factory);
}
} // namespace expense

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

namespace income {
void initializesAddedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto ape{addObservableTransactionStub(factory)};
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                ape->initializedTransaction());
  });
}
} // namespace income

namespace expense {
void initializesAddedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla{addObservableTransactionStub(factory)};
    credit(account,
           Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertEqual(
        result,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
        gorilla->initializedTransaction());
  });
}

void notifiesObserverOfRemoval(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.remove();
        assertTrue(result, observer.willBeRemoved());
      });
}
} // namespace expense

namespace income {
void notifiesObserverOfRemoval(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.remove();
        assertTrue(result, observer.willBeRemoved());
      });
}

void notifiesObserverOfNewCredit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto record{addObservableTransactionStub(factory)};
    credit(account);
    assertEqual(result, record.get(), observer.newTransactionRecord());
  });
}
} // namespace income

namespace expense {
void notifiesObserverOfNewDebit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto record{addObservableTransactionStub(factory)};
    debit(account);
    assertEqual(result, record.get(), observer.newTransactionRecord());
  });
}
} // namespace expense

namespace income {
void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    account.deposit(5_cents);
    addObservableTransactionStub(factory)->setAmount(3_cents);
    credit(account);
    assertBalanceEquals(result, 5_cents + 3_cents, observer);
    addObservableTransactionStub(factory)->setAmount(11_cents);
    credit(account);
    assertBalanceEquals(result, 5_cents + 3_cents + 11_cents, observer);
  });
}
} // namespace income

namespace income {
void savesAllTransactionsAndAccountName(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto john{addObservableTransactionStub(factory)};
    credit(account);
    const auto andy{addObservableTransactionStub(factory)};
    credit(account);
    PersistentAccountStub persistence;
    account.save(persistence);
    assertCreditsSaved(result, persistence, {john.get(), andy.get()});
  });
}
} // namespace income

namespace income {
void attemptsToRemoveEachCreditUntilFound(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
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
} // namespace income

namespace income {
void savesLoadedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    TransactionDeserializationStub deserialization;
    const auto mike{addObservableTransactionStub(factory)};
    account.notifyThatIsReady(deserialization);
    const auto joe{addObservableTransactionStub(factory)};
    account.notifyThatIsReady(deserialization);
    PersistentAccountStub persistence;
    account.save(persistence);
    assertCreditsSaved(result, persistence, {mike.get(), joe.get()});
  });
}
} // namespace income

namespace income {
void savesRemainingTransactionsAfterRemovingSome(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto ape{addObservableTransactionStub(factory)};
    credit(account);
    const auto orangutan{addObservableTransactionStub(factory)};
    credit(account);
    orangutan->setRemoves();
    account.remove({});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertCreditsSaved(result, persistence, {ape.get()});
  });
}
} // namespace income

namespace income {
void savesRemainingTransactionAfterRemovingVerified(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla{addObservableTransactionInMemory(factory)};
    credit(account,
           Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto chimpanzee{addObservableTransactionInMemory(factory)};
    credit(account,
           Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    account.verify(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.remove(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertCreditsSaved(result, persistence, {chimpanzee.get()});
  });
}
} // namespace income

namespace income {
void savesDuplicateTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla1{addObservableTransactionInMemory(factory)};
    credit(account,
           Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto gorilla2{addObservableTransactionInMemory(factory)};
    credit(account,
           Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertCreditsSaved(result, persistence, {gorilla1.get(), gorilla2.get()});
  });
}
} // namespace income

namespace income {
void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    factory.add(std::make_shared<ObservableTransactionInMemory>());
    factory.add(std::make_shared<ObservableTransactionInMemory>());
    AccountObserverStub observer;
    account.attach(&observer);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    credit(account,
           Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    account.remove(
        Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    assertBalanceEquals(result, 123_cents, observer);
  });
}
} // namespace income

namespace income {
void observesDeserialization(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account, ObservableTransactionFactoryStub &) {
        PersistentAccountStub persistence;
        account.load(persistence);
        assertEqual(result,
                    static_cast<AccountDeserialization::Observer *>(&account),
                    persistence.observer());
      });
}

void hasTransactionsObserveDeserialization(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{addObservableTransactionStub(factory)};
    TransactionDeserializationStub abel;
    account.notifyThatIsReady(abel);
    assertEqual(result, mike.get(), abel.observer());
  });
}
} // namespace income

namespace income {
void notifiesObserverThatDuplicateTransactionsAreVerified(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla1{addObservableTransactionInMemory(factory)};
    TransactionObserverStub gorilla1Observer;
    gorilla1->attach(&gorilla1Observer);
    credit(account,
           Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto gorilla2{addObservableTransactionInMemory(factory)};
    TransactionObserverStub gorilla2Observer;
    gorilla2->attach(&gorilla2Observer);
    credit(account,
           Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verify(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verify(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertTrue(result, gorilla1Observer.verified());
    assertTrue(result, gorilla2Observer.verified());
  });
}
} // namespace income

namespace income {
void notifiesObserverOfVerifiedCredit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{addObservableTransactionInMemory(factory)};
    TransactionObserverStub observer;
    record->attach(&observer);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verify(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void notifiesObserverOfRemovedCredit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{addObservableTransactionInMemory(factory)};
    TransactionObserverStub observer;
    record->attach(&observer);
    account.add(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.remove(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}
} // namespace income

namespace income {
void reducesTransactionsToFunds(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    addObservableTransactionInMemory(factory);
    credit(account, Transaction{2300_cents, "orangutan",
                                Date{2020, Month::February, 2}});
    account.reduce();
    assertEqual(result, 2300_cents + 789_cents, account.balance());
  });
}
} // namespace income

namespace income {
void clearsReducedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{2_cents, "chimpanzee", Date{2020, Month::June, 1}});
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{1_cents, "orangutan", Date{2020, Month::February, 2}});
    account.reduce();
    PersistentAccountStub persistence;
    account.save(persistence);
    assertCreditsSaved(result, persistence, {});
  });
}
} // namespace income

namespace income {
void removesTransactionsWhenReducing(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto orangutan{addObservableTransactionStub(factory)};
    credit(account, Transaction{2300_cents, "orangutan",
                                Date{2020, Month::February, 2}});
    account.reduce();
    assertTrue(result, orangutan->removed());
  });
}
} // namespace income

namespace income {
void returnsBalance(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    account.deposit(4_cents);
    addObservableTransactionInMemory(factory);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    assertEqual(result, 4_cents + 123_cents + 111_cents, account.balance());
  });
}
} // namespace income

namespace income {
void notifiesObserverOfUpdatedFundsAndBalanceOnClear(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    account.deposit(1_cents);
    addObservableTransactionInMemory(factory);
    credit(account, Transaction{2_cents, "a", Date{}});
    account.clear();
    assertEqual(result, 0_cents, observer.balance());
  });
}
} // namespace income
} // namespace sbash64::budget::account
