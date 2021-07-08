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
class TransactionRecordStub : public TransactionRecord {
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

  void save(TransactionRecordSerialization &) override {}

  void load(TransactionRecordDeserialization &) override {}

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

class TransactionRecordFactoryStub : public TransactionRecord::Factory {
public:
  void add(std::shared_ptr<TransactionRecord> record) {
    transactionRecords.push_back(std::move(record));
  }

  auto make() -> std::shared_ptr<TransactionRecord> override {
    auto next{transactionRecords.front()};
    transactionRecords.erase(transactionRecords.begin());
    return next;
  }

private:
  std::vector<std::shared_ptr<TransactionRecord>> transactionRecords;
  Transaction transaction_;
};

class TransactionRecordSerializationStub
    : public TransactionRecordSerialization {
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

class TransactionRecordObserverStub : public TransactionRecord::Observer {
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

  void notifyThatCreditHasBeenAdded(TransactionRecord &tr) override {
    newTransactionRecord_ = &tr;
  }

  auto newTransactionRecord() -> const TransactionRecord * {
    return newTransactionRecord_;
  }

  void notifyThatDebitHasBeenAdded(TransactionRecord &tr) override {
    newTransactionRecord_ = &tr;
  }

  void notifyThatWillBeRemoved() override {}

private:
  TransactionRecord *newTransactionRecord_{};
  USD balance_{};
};

class TransactionRecordDeserializationStub
    : public TransactionRecordDeserialization {
public:
  void load(Observer &a) override { observer_ = &a; }

  auto observer() -> Observer * { return observer_; }

private:
  Observer *observer_{};
};
} // namespace

constexpr auto to_integral(Transaction::Type e) ->
    typename std::underlying_type<Transaction::Type>::type {
  return static_cast<typename std::underlying_type<Transaction::Type>::type>(e);
}

static void assertContains(testcpplite::TestResult &result,
                           const std::vector<TransactionRecord *> &transactions,
                           const TransactionRecord *transaction) {
  assertTrue(result, std::find(transactions.begin(), transactions.end(),
                               transaction) != transactions.end());
}

static void assertContainsCredit(testcpplite::TestResult &result,
                                 PersistentAccountStub &persistentMemory,
                                 const TransactionRecord *transaction) {
  assertContains(result, persistentMemory.credits(), transaction);
}

static void assertContainsDebit(testcpplite::TestResult &result,
                                PersistentAccountStub &persistentMemory,
                                const TransactionRecord *transaction) {
  assertContains(result, persistentMemory.debits(), transaction);
}

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<TransactionRecord *> &expected,
                        const std::vector<TransactionRecord *> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<TransactionRecord *>::size_type i{0}; i < expected.size();
       ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

static void
assertDebitsSaved(testcpplite::TestResult &result,
                  PersistentAccountStub &persistentMemory,
                  const std::vector<TransactionRecord *> &transactions) {
  assertEqual(result, persistentMemory.debits(), transactions);
}

static void
assertCreditsSaved(testcpplite::TestResult &result,
                   PersistentAccountStub &persistentMemory,
                   const std::vector<TransactionRecord *> &transactions) {
  assertEqual(result, persistentMemory.credits(), transactions);
}

static void credit(Account &account, const Transaction &t) {
  account.credit(t);
}

static void debit(Account &account, const Transaction &t) { account.debit(t); }

static void assertAccountName(testcpplite::TestResult &result,
                              PersistentAccountStub &persistent,
                              std::string_view expected) {
  assertEqual(result, std::string{expected}, persistent.accountName());
}

static void testAccount(
    const std::function<void(InMemoryAccount &, TransactionRecordFactoryStub &)>
        &f,
    std::string name = {}) {
  TransactionRecordFactoryStub transactionRecordFactory;
  InMemoryAccount account{std::move(name), transactionRecordFactory};
  f(account, transactionRecordFactory);
}

static void testAccount(const std::function<void(InMemoryAccount &)> &f,
                        std::string name = {}) {
  TransactionRecordFactoryStub transactionRecordFactory;
  InMemoryAccount account{std::move(name), transactionRecordFactory};
  f(account);
}

void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        const auto joe{std::make_shared<TransactionRecordStub>()};
        const auto mike{std::make_shared<TransactionRecordStub>()};
        const auto andy{std::make_shared<TransactionRecordStub>()};
        factory.add(joe);
        factory.add(mike);
        factory.add(andy);
        joe->setAmount(3_cents);
        mike->setAmount(5_cents);
        andy->setAmount(11_cents);
        AccountObserverStub observer;
        account.attach(&observer);
        credit(account, {});
        assertEqual(result, 3_cents, observer.balance());
        debit(account, {});
        assertEqual(result, 3_cents - 5_cents, observer.balance());
        credit(account, {});
        assertEqual(result, 3_cents - 5_cents + 11_cents, observer.balance());
      });
}

void notifiesObserverOfUpdatedBalanceAfterRemovingTransactions(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    for (auto i{0}; i < 4; ++i)
      factory.add(std::make_shared<TransactionRecordInMemory>());
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
    assertEqual(result, 123_cents + 111_cents - 789_cents, observer.balance());
    account.removeCredit(
        Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    assertEqual(result, 123_cents - 789_cents, observer.balance());
  });
}

void savesAllTransactionRecordsAndAccountName(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        const auto john{std::make_shared<TransactionRecordStub>()};
        const auto mike{std::make_shared<TransactionRecordStub>()};
        const auto andy{std::make_shared<TransactionRecordStub>()};
        factory.add(john);
        factory.add(mike);
        factory.add(andy);
        credit(account, {});
        debit(account, {});
        credit(account, {});
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertAccountName(result, persistentMemory, "joe");
        assertCreditsSaved(result, persistentMemory, {john.get(), andy.get()});
        assertDebitsSaved(result, persistentMemory, {mike.get()});
      },
      "joe");
}

void savesRemainingTransactionRecordsAfterRemovingSome(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto mike{std::make_shared<TransactionRecordInMemory>()};
    const auto andy{std::make_shared<TransactionRecordInMemory>()};
    const auto joe{std::make_shared<TransactionRecordInMemory>()};
    const auto bob{std::make_shared<TransactionRecordInMemory>()};
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
    account.removeDebit(
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    account.removeCredit(
        Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
    PersistentAccountStub persistentMemory;
    account.save(persistentMemory);
    assertCreditsSaved(result, persistentMemory, {mike.get()});
    assertDebitsSaved(result, persistentMemory, {bob.get()});
  });
}

void initializesTransactionRecords(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto joe{std::make_shared<TransactionRecordStub>()};
    const auto mike{std::make_shared<TransactionRecordStub>()};
    const auto andy{std::make_shared<TransactionRecordStub>()};
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
  testAccount([&](InMemoryAccount &account) {
    PersistentAccountStub persistentMemory;
    account.load(persistentMemory);
    assertEqual(result, &account, persistentMemory.observer());
  });
}

void passesNewTransactionRecordsToDeserialization(
    testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        const auto mike{std::make_shared<TransactionRecordStub>()};
        const auto joe{std::make_shared<TransactionRecordStub>()};
        factory.add(mike);
        factory.add(joe);
        TransactionRecordDeserializationStub abel;
        account.notifyThatCreditIsReady(abel);
        assertEqual(result, mike.get(), abel.observer());
        account.notifyThatDebitIsReady(abel);
        assertEqual(result, joe.get(), abel.observer());
      });
}

void savesTransactionRecordsLoaded(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        const auto mike{std::make_shared<TransactionRecordStub>()};
        const auto andy{std::make_shared<TransactionRecordStub>()};
        const auto joe{std::make_shared<TransactionRecordStub>()};
        const auto bob{std::make_shared<TransactionRecordStub>()};
        factory.add(mike);
        factory.add(andy);
        factory.add(joe);
        factory.add(bob);
        TransactionRecordDeserializationStub abel;
        account.notifyThatCreditIsReady(abel);
        account.notifyThatDebitIsReady(abel);
        account.notifyThatCreditIsReady(abel);
        account.notifyThatDebitIsReady(abel);
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertDebitsSaved(result, persistentMemory, {andy.get(), bob.get()});
        assertCreditsSaved(result, persistentMemory, {mike.get(), joe.get()});
      });
}

void rename(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account) {
        account.rename("mike");
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertAccountName(result, persistentMemory, "mike");
      },
      "joe");
}

void savesRemainingTransactionRecordsAfterRemovingVerified(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto mike{std::make_shared<TransactionRecordInMemory>()};
    const auto andy{std::make_shared<TransactionRecordInMemory>()};
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
    PersistentAccountStub persistentMemory;
    account.save(persistentMemory);
    assertDebitsSaved(result, persistentMemory, {andy.get()});
  });
}

void notifiesDuplicateTransactionsAreVerified(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    TransactionRecordObserverStub john;
    TransactionRecordObserverStub alex;
    const auto mike{std::make_shared<TransactionRecordInMemory>()};
    const auto andy{std::make_shared<TransactionRecordInMemory>()};
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
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto mike{std::make_shared<TransactionRecordInMemory>()};
    const auto andy{std::make_shared<TransactionRecordInMemory>()};
    factory.add(mike);
    factory.add(andy);
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    PersistentAccountStub persistentMemory;
    account.save(persistentMemory);
    assertDebitsSaved(result, persistentMemory, {mike.get(), andy.get()});
  });
}

void notifiesObserverOfNewCredit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto record{std::make_shared<TransactionRecordStub>()};
    factory.add(record);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result, record.get(), observer.newTransactionRecord());
  });
}

void notifiesObserverOfNewDebit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto record{std::make_shared<TransactionRecordStub>()};
    factory.add(record);
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result, record.get(), observer.newTransactionRecord());
  });
}

void notifiesCreditIsVerified(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto record{std::make_shared<TransactionRecordInMemory>()};
    factory.add(record);
    TransactionRecordObserverStub observer;
    record->attach(&observer);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void notifiesDebitIsVerified(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto record{std::make_shared<TransactionRecordInMemory>()};
    factory.add(record);
    TransactionRecordObserverStub observer;
    record->attach(&observer);
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyDebit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void notifiesObserverOfRemovedDebit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto record{std::make_shared<TransactionRecordInMemory>()};
    factory.add(record);
    TransactionRecordObserverStub observer;
    record->attach(&observer);
    account.debit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeDebit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}

void notifiesObserverOfRemovedCredit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto record{std::make_shared<TransactionRecordInMemory>()};
    factory.add(record);
    TransactionRecordObserverStub observer;
    record->attach(&observer);
    account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}

void reduceReducesToOneTransaction(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto mike{std::make_shared<TransactionRecordInMemory>()};
    const auto andy{std::make_shared<TransactionRecordInMemory>()};
    const auto joe{std::make_shared<TransactionRecordInMemory>()};
    const auto bob{std::make_shared<TransactionRecordStub>()};
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
  TransactionRecordInMemory record;
  TransactionRecordObserverStub observer;
  record.attach(&observer);
  record.verify();
  assertTrue(result, observer.verified());
}

void saveAfterVerify(testcpplite::TestResult &result) {
  TransactionRecordInMemory record;
  record.verify();
  TransactionRecordSerializationStub serialization;
  record.save(serialization);
  assertTrue(result, serialization.verifiableTransaction().verified);
}

void notifiesObserverWhenRemoved(testcpplite::TestResult &result) {
  TransactionRecordInMemory record;
  TransactionRecordObserverStub observer;
  record.attach(&observer);
  record.remove();
  assertTrue(result, observer.removed());
}

void notifiesObserverOfTransactionsWhenReducing(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto mike{std::make_shared<TransactionRecordStub>()};
    const auto andy{std::make_shared<TransactionRecordStub>()};
    const auto joe{std::make_shared<TransactionRecordStub>()};
    const auto bob{std::make_shared<TransactionRecordStub>()};
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
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto mike{std::make_shared<TransactionRecordInMemory>()};
    const auto andy{std::make_shared<TransactionRecordInMemory>()};
    const auto joe{std::make_shared<TransactionRecordInMemory>()};
    const auto bob{std::make_shared<TransactionRecordInMemory>()};
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
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    const auto mike{std::make_shared<TransactionRecordInMemory>()};
    const auto andy{std::make_shared<TransactionRecordInMemory>()};
    const auto joe{std::make_shared<TransactionRecordInMemory>()};
    const auto bob{std::make_shared<TransactionRecordStub>()};
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
