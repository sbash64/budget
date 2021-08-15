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

  void notifyThatCreditHasBeenAdded(ObservableTransaction &tr) override {
    newTransactionRecord_ = &tr;
  }

  auto newTransactionRecord() -> const ObservableTransaction * {
    return newTransactionRecord_;
  }

  void notifyThatDebitHasBeenAdded(ObservableTransaction &tr) override {
    newTransactionRecord_ = &tr;
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
assertDebitsSaved(testcpplite::TestResult &result,
                  PersistentAccountStub &persistence,
                  const std::vector<SerializableTransaction *> &transactions) {
  assertEqual(result, persistence.debits(), transactions);
}

static void
assertCreditsSaved(testcpplite::TestResult &result,
                   PersistentAccountStub &persistence,
                   const std::vector<SerializableTransaction *> &transactions) {
  assertEqual(result, persistence.credits(), transactions);
}

static void credit(IncomeAccount &account, const Transaction &t = {}) {
  account.credit(t);
}

static void debit(ExpenseAccount &account, const Transaction &t = {}) {
  account.debit(t);
}

static void assertAccountName(testcpplite::TestResult &result,
                              PersistentAccountStub &persistent,
                              std::string_view expected) {
  assertEqual(result, std::string{expected}, persistent.accountName());
}

static void testInMemoryAccount(
    const std::function<void(InMemoryAccount &,
                             ObservableTransactionFactoryStub &)> &test,
    std::string name = {}) {
  ObservableTransactionFactoryStub factory;
  InMemoryAccount account{std::move(name), factory};
  test(account, factory);
}

namespace expense {
static void testInMemoryAccount(
    const std::function<void(InMemoryExpenseAccount &,
                             ObservableTransactionFactoryStub &)> &test,
    std::string name = {}) {
  ObservableTransactionFactoryStub factory;
  InMemoryExpenseAccount account{std::move(name), factory};
  test(account, factory);
}
} // namespace expense

static void
testInMemoryAccount(const std::function<void(InMemoryAccount &)> &test,
                    std::string name = {}) {
  ObservableTransactionFactoryStub factory;
  InMemoryAccount account{std::move(name), factory};
  test(account);
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
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto ape{addObservableTransactionStub(factory)};
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                ape->initializedTransaction());
    const auto gorilla{addObservableTransactionStub(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertEqual(
        result,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
        gorilla->initializedTransaction());
  });
}

namespace expense {
void initializesAddedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla{addObservableTransactionStub(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertEqual(
        result,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
        gorilla->initializedTransaction());
  });
}
} // namespace expense

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

namespace expense {
void notifiesObserverOfNewDebit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryExpenseAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto record{addObservableTransactionStub(factory)};
    debit(account);
    assertEqual(result, record.get(), observer.newTransactionRecord());
  });
}
} // namespace expense

void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    addObservableTransactionStub(factory)->setAmount(3_cents);
    credit(account);
    assertBalanceEquals(result, 3_cents, observer);
    addObservableTransactionStub(factory)->setAmount(5_cents);
    debit(account);
    assertBalanceEquals(result, 3_cents - 5_cents, observer);
    addObservableTransactionStub(factory)->setAmount(11_cents);
    credit(account);
    assertBalanceEquals(result, 3_cents - 5_cents + 11_cents, observer);
  });
}

namespace expense {
void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryExpenseAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    addObservableTransactionStub(factory)->setAmount(5_cents);
    debit(account);
    assertBalanceEquals(result, -5_cents, observer);
  });
}
} // namespace expense

void savesAllTransactionsAndAccountName(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account,
                ObservableTransactionFactoryStub &factory) {
        const auto john{addObservableTransactionStub(factory)};
        credit(account);
        const auto mike{addObservableTransactionStub(factory)};
        debit(account);
        const auto andy{addObservableTransactionStub(factory)};
        credit(account);
        account.deposit(1_cents);
        PersistentAccountStub persistence;
        account.save(persistence);
        assertAccountName(result, persistence, "joe");
        assertEqual(result, 1_cents, persistence.funds());
        assertCreditsSaved(result, persistence, {john.get(), andy.get()});
        assertDebitsSaved(result, persistence, {mike.get()});
      },
      "joe");
}

namespace expense {
void savesAllTransactionsAndAccountName(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryExpenseAccount &account,
                ObservableTransactionFactoryStub &factory) {
        const auto mike{addObservableTransactionStub(factory)};
        debit(account);
        account.deposit(1_cents);
        PersistentAccountStub persistence;
        account.save(persistence);
        assertAccountName(result, persistence, "joe");
        assertEqual(result, 1_cents, persistence.funds());
        assertDebitsSaved(result, persistence, {mike.get()});
      },
      "joe");
}
} // namespace expense

namespace expense {
void attemptsToRemoveEachDebitUntilFound(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryExpenseAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    TransactionDeserializationStub deserialization;
    const auto mike{addObservableTransactionStub(factory)};
    account.notifyThatDebitIsReady(deserialization);
    const auto andy{addObservableTransactionStub(factory)};
    account.notifyThatDebitIsReady(deserialization);
    const auto joe{addObservableTransactionStub(factory)};
    account.notifyThatDebitIsReady(deserialization);
    const auto bob{addObservableTransactionStub(factory)};
    account.notifyThatDebitIsReady(deserialization);
    joe->setRemoves();
    Transaction transaction;
    account.removeDebit(transaction);
    assertEqual(result, &transaction, mike->removesTransaction());
    assertEqual(result, &transaction, andy->removesTransaction());
    assertEqual(result, &transaction, joe->removesTransaction());
    assertFalse(result, bob->removesed());
  });
}
} // namespace expense

void attemptsToRemoveEachCreditUntilFound(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    TransactionDeserializationStub deserialization;
    const auto mike{addObservableTransactionStub(factory)};
    account.notifyThatCreditIsReady(deserialization);
    const auto andy{addObservableTransactionStub(factory)};
    account.notifyThatCreditIsReady(deserialization);
    const auto joe{addObservableTransactionStub(factory)};
    account.notifyThatCreditIsReady(deserialization);
    const auto bob{addObservableTransactionStub(factory)};
    account.notifyThatCreditIsReady(deserialization);
    andy->setRemoves();
    Transaction transaction;
    account.removeCredit(transaction);
    assertEqual(result, &transaction, mike->removesTransaction());
    assertEqual(result, &transaction, andy->removesTransaction());
    assertFalse(result, joe->removesed());
    assertFalse(result, bob->removesed());
  });
}

void savesLoadedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    TransactionDeserializationStub deserialization;
    const auto mike{addObservableTransactionStub(factory)};
    account.notifyThatCreditIsReady(deserialization);
    const auto andy{addObservableTransactionStub(factory)};
    account.notifyThatDebitIsReady(deserialization);
    const auto joe{addObservableTransactionStub(factory)};
    account.notifyThatCreditIsReady(deserialization);
    const auto bob{addObservableTransactionStub(factory)};
    account.notifyThatDebitIsReady(deserialization);
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {andy.get(), bob.get()});
    assertCreditsSaved(result, persistence, {mike.get(), joe.get()});
  });
}

namespace expense {
void savesLoadedTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryExpenseAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    TransactionDeserializationStub deserialization;
    const auto andy{addObservableTransactionStub(factory)};
    account.notifyThatDebitIsReady(deserialization);
    const auto bob{addObservableTransactionStub(factory)};
    account.notifyThatDebitIsReady(deserialization);
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {andy.get(), bob.get()});
  });
}
} // namespace expense

void savesRemainingTransactionsAfterRemovingSome(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto ape{addObservableTransactionStub(factory)};
    credit(account);
    const auto gorilla{addObservableTransactionStub(factory)};
    debit(account);
    const auto orangutan{addObservableTransactionStub(factory)};
    credit(account);
    const auto chimpanzee{addObservableTransactionStub(factory)};
    debit(account);
    gorilla->setRemoves();
    account.removeDebit({});
    orangutan->setRemoves();
    account.removeCredit({});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertCreditsSaved(result, persistence, {ape.get()});
    assertDebitsSaved(result, persistence, {chimpanzee.get()});
  });
}

namespace expense {
void savesRemainingTransactionsAfterRemovingSome(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryExpenseAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla{addObservableTransactionStub(factory)};
    debit(account);
    const auto chimpanzee{addObservableTransactionStub(factory)};
    debit(account);
    gorilla->setRemoves();
    account.removeDebit({});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {chimpanzee.get()});
  });
}
} // namespace expense

void savesRemainingTransactionAfterRemovingVerified(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla{addObservableTransactionInMemory(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto chimpanzee{addObservableTransactionInMemory(factory)};
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    account.verifyDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.removeDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {chimpanzee.get()});
  });
}

namespace expense {
void savesRemainingTransactionAfterRemovingVerified(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryExpenseAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla{addObservableTransactionInMemory(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto chimpanzee{addObservableTransactionInMemory(factory)};
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    account.verifyDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.removeDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {chimpanzee.get()});
  });
}
} // namespace expense

void savesDuplicateTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla1{addObservableTransactionInMemory(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto gorilla2{addObservableTransactionInMemory(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {gorilla1.get(), gorilla2.get()});
  });
}

namespace expense {
void savesDuplicateTransactions(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryExpenseAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla1{addObservableTransactionInMemory(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto gorilla2{addObservableTransactionInMemory(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {gorilla1.get(), gorilla2.get()});
  });
}
} // namespace expense

void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    for (auto i{0}; i < 4; ++i)
      factory.add(std::make_shared<ObservableTransactionInMemory>());
    AccountObserverStub observer;
    account.attach(&observer);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    credit(account,
           Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    account.removeDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertBalanceEquals(result, 123_cents + 111_cents - 789_cents, observer);
    account.removeCredit(
        Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    assertBalanceEquals(result, 123_cents - 789_cents, observer);
  });
}

void observesDeserialization(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account) {
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
    const auto joe{addObservableTransactionStub(factory)};
    TransactionDeserializationStub abel;
    account.notifyThatCreditIsReady(abel);
    assertEqual(result, mike.get(), abel.observer());
    account.notifyThatDebitIsReady(abel);
    assertEqual(result, joe.get(), abel.observer());
  });
}

void savesNewName(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account) {
        account.rename("mike");
        PersistentAccountStub persistence;
        account.save(persistence);
        assertAccountName(result, persistence, "mike");
      },
      "joe");
}

void notifiesObserverThatDuplicateTransactionsAreVerified(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla1{addObservableTransactionInMemory(factory)};
    TransactionObserverStub gorilla1Observer;
    gorilla1->attach(&gorilla1Observer);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto gorilla2{addObservableTransactionInMemory(factory)};
    TransactionObserverStub gorilla2Observer;
    gorilla2->attach(&gorilla2Observer);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verifyDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verifyDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertTrue(result, gorilla1Observer.verified());
    assertTrue(result, gorilla2Observer.verified());
  });
}

void notifiesObserverOfVerifiedCredit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{addObservableTransactionInMemory(factory)};
    TransactionObserverStub observer;
    record->attach(&observer);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void notifiesObserverOfVerifiedDebit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{addObservableTransactionInMemory(factory)};
    TransactionObserverStub observer;
    record->attach(&observer);
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyDebit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void notifiesObserverOfRemovedDebit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{addObservableTransactionInMemory(factory)};
    TransactionObserverStub observer;
    record->attach(&observer);
    account.debit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeDebit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}

void notifiesObserverOfRemovedCredit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{addObservableTransactionInMemory(factory)};
    TransactionObserverStub observer;
    record->attach(&observer);
    account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}

void reducesToOneTransaction(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    addObservableTransactionInMemory(factory);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    addObservableTransactionInMemory(factory);
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    addObservableTransactionInMemory(factory);
    credit(account, Transaction{2300_cents, "orangutan",
                                Date{2020, Month::February, 2}});
    const auto reduction{addObservableTransactionStub(factory)};
    account.reduce();
    assertEqual(result, 2300_cents - 789_cents - 456_cents, account.balance());
  });
}

void reducesToOneDebitForNegativeBalance(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    addObservableTransactionInMemory(factory);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    addObservableTransactionInMemory(factory);
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{300_cents, "orangutan", Date{2020, Month::February, 2}});
    const auto reduction{addObservableTransactionStub(factory)};
    account.reduce();
    assertEqual(result, -789_cents - 456_cents + 300_cents, account.balance());
  });
}

void reducesToOneCreditForPositiveBalance(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    addObservableTransactionInMemory(factory);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{300_cents, "orangutan", Date{2020, Month::February, 2}});
    const auto reduction{addObservableTransactionStub(factory)};
    account.reduce();
    assertEqual(result, 789_cents + 300_cents - 456_cents, account.balance());
  });
}

void reducesToNoTransactionsForZeroBalance(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    addObservableTransactionInMemory(factory);
    debit(account,
          Transaction{3_cents, "gorilla", Date{2020, Month::January, 20}});
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{2_cents, "chimpanzee", Date{2020, Month::June, 1}});
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{1_cents, "orangutan", Date{2020, Month::February, 2}});
    account.reduce();
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {});
    assertCreditsSaved(result, persistence, {});
  });
}

void removesTransactionsWhenReducing(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto gorilla{addObservableTransactionStub(factory)};
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    const auto chimpanzee{addObservableTransactionStub(factory)};
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    const auto orangutan{addObservableTransactionStub(factory)};
    credit(account, Transaction{2300_cents, "orangutan",
                                Date{2020, Month::February, 2}});
    addObservableTransactionStub(factory);
    account.reduce();
    assertTrue(result, gorilla->removed());
    assertTrue(result, chimpanzee->removed());
    assertTrue(result, orangutan->removed());
  });
}

void returnsBalance(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    addObservableTransactionInMemory(factory);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    addObservableTransactionInMemory(factory);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    addObservableTransactionInMemory(factory);
    credit(account,
           Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    addObservableTransactionInMemory(factory);
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertEqual(result, 123_cents + 111_cents - 789_cents - 456_cents,
                account.balance());
  });
}

void withdrawsFromFunds(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.withdraw(12_cents);
        assertEqual(result, -12_cents, observer.balance());
      });
}

void depositsToFunds(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.deposit(12_cents);
        assertEqual(result, 12_cents, observer.balance());
      });
}

void notifiesObserverOfUpdatedFundsOnDeposit(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.deposit(12_cents);
        assertEqual(result, 12_cents, observer.funds());
      });
}

void notifiesObserverOfUpdatedFundsOnWithdraw(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.withdraw(12_cents);
        assertEqual(result, -12_cents, observer.funds());
      });
}

void notifiesObserverOfUpdatedFundsOnReduce(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    addObservableTransactionInMemory(factory);
    credit(account, Transaction{1_cents, "a", Date{}});
    addObservableTransactionInMemory(factory);
    debit(account, Transaction{2_cents, "a", Date{}});
    addObservableTransactionInMemory(factory);
    credit(account, Transaction{3_cents, "a", Date{}});
    account.deposit(4_cents);
    account.reduce();
    assertEqual(result, 1_cents - 2_cents + 3_cents + 4_cents,
                observer.funds());
  });
}

void notifiesObserverOfUpdatedFundsAndBalanceOnClear(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    account.deposit(1_cents);
    addObservableTransactionInMemory(factory);
    debit(account, Transaction{2_cents, "a", Date{}});
    account.clear();
    assertEqual(result, 0_cents, observer.funds());
    assertEqual(result, 0_cents, observer.balance());
  });
}

void notifiesObserverOfUpdatedFundsAndBalanceOnSerialization(
    testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account, ObservableTransactionFactoryStub &) {
        AccountObserverStub observer;
        account.attach(&observer);
        account.notifyThatFundsAreReady(1_cents);
        assertEqual(result, 1_cents, observer.funds());
        assertEqual(result, 1_cents, observer.balance());
      });
}
} // namespace sbash64::budget::account
