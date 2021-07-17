#include "account.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"

#include <sbash64/budget/account.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

#include <functional>
#include <map>
#include <utility>

namespace sbash64::budget::account {
namespace {
class ObservableTransactionStub : public ObservableTransaction {
public:
  void verify() override { verified_ = true; }

  [[nodiscard]] auto verified() const -> bool { return verified_; }

  void ready(const VerifiableTransaction &) override {}

  void attach(Observer *) override {}

  void initialize(const Transaction &t) override {
    initializedTransaction_ = t;
  }

  auto initializedTransaction() -> Transaction {
    return initializedTransaction_;
  }

  auto verifies(const Transaction &) -> bool override { return {}; }

  auto removes(const Transaction &) -> bool override { return {}; }

  void save(TransactionSerialization &) override {}

  void load(TransactionDeserialization &) override {}

  auto amount() -> USD override { return amount_; }

  void setAmount(USD x) { amount_ = x; }

  void remove() override { removed_ = true; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

private:
  Transaction initializedTransaction_;
  USD amount_;
  bool verified_{};
  bool removed_{};
};

class ObservableTransactionFactoryStub : public ObservableTransaction::Factory {
public:
  void add(std::shared_ptr<ObservableTransaction> another) {
    observableTransactions.push_back(std::move(another));
  }

  auto make() -> std::shared_ptr<ObservableTransaction> override {
    auto next{observableTransactions.front()};
    observableTransactions.erase(observableTransactions.begin());
    return next;
  }

private:
  std::vector<std::shared_ptr<ObservableTransaction>> observableTransactions;
  Transaction transaction_;
};

class TransactionSerializationStub : public TransactionSerialization {
public:
  void save(const VerifiableTransaction &vt) override {
    verifiableTransaction_ = vt;
  }

  auto verifiableTransaction() -> VerifiableTransaction {
    return verifiableTransaction_;
  }

private:
  VerifiableTransaction verifiableTransaction_;
};

class TransactionObserverStub : public ObservableTransaction::Observer {
public:
  void notifyThatIsVerified() override { verified_ = true; }

  void notifyThatIs(const Transaction &t) override { transaction_ = t; }

  auto transaction() -> Transaction { return transaction_; }

  [[nodiscard]] auto verified() const -> bool { return verified_; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

  void notifyThatWillBeRemoved() override { removed_ = true; }

private:
  Transaction transaction_;
  bool verified_{};
  bool removed_{};
};

class AccountObserverStub : public Account::Observer {
public:
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

static void credit(Account &account, const Transaction &t = {}) {
  account.credit(t);
}

static void debit(Account &account, const Transaction &t = {}) {
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

void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto threeCents{std::make_shared<ObservableTransactionStub>()};
    const auto fiveCents{std::make_shared<ObservableTransactionStub>()};
    const auto elevenCents{std::make_shared<ObservableTransactionStub>()};
    factory.add(threeCents);
    factory.add(fiveCents);
    factory.add(elevenCents);
    threeCents->setAmount(3_cents);
    fiveCents->setAmount(5_cents);
    elevenCents->setAmount(11_cents);
    AccountObserverStub observer;
    account.attach(&observer);
    credit(account);
    assertBalanceEquals(result, 3_cents, observer);
    debit(account);
    assertBalanceEquals(result, 3_cents - 5_cents, observer);
    credit(account);
    assertBalanceEquals(result, 3_cents - 5_cents + 11_cents, observer);
  });
}

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

void savesAllTransactionsAndAccountName(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account,
                ObservableTransactionFactoryStub &factory) {
        const auto john{std::make_shared<ObservableTransactionStub>()};
        factory.add(john);
        const auto mike{std::make_shared<ObservableTransactionStub>()};
        factory.add(mike);
        const auto andy{std::make_shared<ObservableTransactionStub>()};
        factory.add(andy);
        credit(account);
        debit(account);
        credit(account);
        PersistentAccountStub persistence;
        account.save(persistence);
        assertAccountName(result, persistence, "joe");
        assertCreditsSaved(result, persistence, {john.get(), andy.get()});
        assertDebitsSaved(result, persistence, {mike.get()});
      },
      "joe");
}

void savesRemainingTransactionsAfterRemovingSome(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(mike);
    const auto andy{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(andy);
    const auto joe{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(joe);
    const auto bob{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(bob);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    credit(account,
           Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    account.removeDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.removeCredit(
        Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertCreditsSaved(result, persistence, {mike.get()});
    assertDebitsSaved(result, persistence, {bob.get()});
  });
}

void initializesTransactionRecords(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto joe{std::make_shared<ObservableTransactionStub>()};
    const auto mike{std::make_shared<ObservableTransactionStub>()};
    const auto andy{std::make_shared<ObservableTransactionStub>()};
    factory.add(joe);
    factory.add(mike);
    factory.add(andy);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    credit(account,
           Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                joe->initializedTransaction());
    assertEqual(
        result,
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
        andy->initializedTransaction());
    assertEqual(
        result,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
        mike->initializedTransaction());
  });
}

void passesSelfToDeserializationOnLoad(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account) {
    PersistentAccountStub persistence;
    account.load(persistence);
    assertEqual(result, &account, persistence.observer());
  });
}

void passesNewTransactionRecordsToDeserialization(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionStub>()};
    const auto joe{std::make_shared<ObservableTransactionStub>()};
    factory.add(mike);
    factory.add(joe);
    TransactionDeserializationStub abel;
    account.notifyThatCreditIsReady(abel);
    assertEqual(result, mike.get(), abel.observer());
    account.notifyThatDebitIsReady(abel);
    assertEqual(result, joe.get(), abel.observer());
  });
}

void savesTransactionRecordsLoaded(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionStub>()};
    const auto andy{std::make_shared<ObservableTransactionStub>()};
    const auto joe{std::make_shared<ObservableTransactionStub>()};
    const auto bob{std::make_shared<ObservableTransactionStub>()};
    factory.add(mike);
    factory.add(andy);
    factory.add(joe);
    factory.add(bob);
    TransactionDeserializationStub abel;
    account.notifyThatCreditIsReady(abel);
    account.notifyThatDebitIsReady(abel);
    account.notifyThatCreditIsReady(abel);
    account.notifyThatDebitIsReady(abel);
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {andy.get(), bob.get()});
    assertCreditsSaved(result, persistence, {mike.get(), joe.get()});
  });
}

void rename(testcpplite::TestResult &result) {
  testInMemoryAccount(
      [&result](InMemoryAccount &account) {
        account.rename("mike");
        PersistentAccountStub persistence;
        account.save(persistence);
        assertAccountName(result, persistence, "mike");
      },
      "joe");
}

void savesRemainingTransactionRecordsAfterRemovingVerified(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionInMemory>()};
    const auto andy{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(mike);
    factory.add(andy);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    account.verifyDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.removeDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {andy.get()});
  });
}

void notifiesDuplicateTransactionsAreVerified(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    TransactionObserverStub john;
    TransactionObserverStub alex;
    const auto mike{std::make_shared<ObservableTransactionInMemory>()};
    const auto andy{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(mike);
    factory.add(andy);
    mike->attach(&john);
    andy->attach(&alex);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verifyDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verifyDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertTrue(result, john.verified());
    assertTrue(result, alex.verified());
  });
}

void savesDuplicateTransactionRecords(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionInMemory>()};
    const auto andy{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(mike);
    factory.add(andy);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistence;
    account.save(persistence);
    assertDebitsSaved(result, persistence, {mike.get(), andy.get()});
  });
}

void notifiesObserverOfNewCredit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto record{std::make_shared<ObservableTransactionStub>()};
    factory.add(record);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result, record.get(), observer.newTransactionRecord());
  });
}

void notifiesObserverOfNewDebit(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto record{std::make_shared<ObservableTransactionStub>()};
    factory.add(record);
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result, record.get(), observer.newTransactionRecord());
  });
}

void notifiesCreditIsVerified(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(record);
    TransactionObserverStub observer;
    record->attach(&observer);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void notifiesDebitIsVerified(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto record{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(record);
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
    const auto record{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(record);
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
    const auto record{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(record);
    TransactionObserverStub observer;
    record->attach(&observer);
    account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}

void reduceReducesToOneTransaction(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionInMemory>()};
    const auto andy{std::make_shared<ObservableTransactionInMemory>()};
    const auto joe{std::make_shared<ObservableTransactionInMemory>()};
    const auto bob{std::make_shared<ObservableTransactionStub>()};
    factory.add(mike);
    factory.add(andy);
    factory.add(joe);
    factory.add(bob);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    credit(account, Transaction{2300_cents, "orangutan",
                                Date{2020, Month::February, 2}});
    account.reduce(Date{2021, Month::March, 13});
    assertEqual(result,
                Transaction{2300_cents - 789_cents - 456_cents, "reduction",
                            Date{2021, Month::March, 13}},
                bob->initializedTransaction());
    assertTrue(result, bob->verified());
  });
}

void notifiesObserverWhenVerified(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionObserverStub observer;
  record.attach(&observer);
  record.verify();
  assertTrue(result, observer.verified());
}

void saveAfterVerify(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  record.verify();
  TransactionSerializationStub serialization;
  record.save(serialization);
  assertTrue(result, serialization.verifiableTransaction().verified);
}

void notifiesObserverWhenRemoved(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionObserverStub observer;
  record.attach(&observer);
  record.remove();
  assertTrue(result, observer.removed());
}

void loadPassesSelfToDeserialization(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionDeserializationStub deserialization;
  record.load(deserialization);
  assertEqual(result, &record, deserialization.observer());
}

void notifiesThatIsAfterReady(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionObserverStub observer;
  record.attach(&observer);
  record.ready(
      {Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, true});
  assertEqual(result,
              Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
              observer.transaction());
  assertTrue(result, observer.verified());
}

void notifiesThatIsAfterInitialize(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionObserverStub observer;
  record.attach(&observer);
  record.initialize(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  assertEqual(result,
              Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
              observer.transaction());
}

void savesWhatWasLoaded(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  record.ready(
      {Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, true});
  TransactionSerializationStub serialization;
  record.save(serialization);
  assertEqual(
      result,
      {Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, true},
      serialization.verifiableTransaction());
}

void notifiesObserverOfTransactionsWhenReducing(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionStub>()};
    const auto andy{std::make_shared<ObservableTransactionStub>()};
    const auto joe{std::make_shared<ObservableTransactionStub>()};
    const auto bob{std::make_shared<ObservableTransactionStub>()};
    factory.add(mike);
    factory.add(andy);
    factory.add(joe);
    factory.add(bob);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    credit(account, Transaction{2300_cents, "orangutan",
                                Date{2020, Month::February, 2}});
    AccountObserverStub observer;
    account.attach(&observer);
    account.reduce(Date{2021, Month::March, 13});
    assertTrue(result, mike->removed());
    assertTrue(result, andy->removed());
    assertTrue(result, joe->removed());
  });
}

void returnsBalance(testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionInMemory>()};
    const auto andy{std::make_shared<ObservableTransactionInMemory>()};
    const auto joe{std::make_shared<ObservableTransactionInMemory>()};
    const auto bob{std::make_shared<ObservableTransactionInMemory>()};
    factory.add(mike);
    factory.add(andy);
    factory.add(joe);
    factory.add(bob);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    credit(account,
           Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertEqual(result, 123_cents + 111_cents - 789_cents - 456_cents,
                account.balance());
  });
}

void reduceReducesToOneDebitForNegativeBalance(
    testcpplite::TestResult &result) {
  testInMemoryAccount([&result](InMemoryAccount &account,
                                ObservableTransactionFactoryStub &factory) {
    const auto mike{std::make_shared<ObservableTransactionInMemory>()};
    const auto andy{std::make_shared<ObservableTransactionInMemory>()};
    const auto joe{std::make_shared<ObservableTransactionInMemory>()};
    const auto bob{std::make_shared<ObservableTransactionStub>()};
    factory.add(mike);
    factory.add(andy);
    factory.add(joe);
    factory.add(bob);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    credit(account,
           Transaction{300_cents, "orangutan", Date{2020, Month::February, 2}});
    account.reduce(Date{2021, Month::March, 13});
    assertEqual(result,
                Transaction{789_cents + 456_cents - 300_cents, "reduction",
                            Date{2021, Month::March, 13}},
                bob->initializedTransaction());
    assertTrue(result, bob->verified());
  });
}
} // namespace sbash64::budget::account
