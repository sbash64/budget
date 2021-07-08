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

  auto removed() -> bool { return removed_; }

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

void showShowsAllTransactionsInChronologicalOrderAndBalance(
    testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        const auto joe{std::make_shared<TransactionRecordStub>()};
        const auto mike{std::make_shared<TransactionRecordStub>()};
        const auto andy{std::make_shared<TransactionRecordStub>()};
        factory.add(joe);
        factory.add(mike);
        factory.add(andy);
        AccountObserverStub observer;
        account.attach(&observer);
        joe->setAmount(3_cents);
        mike->setAmount(5_cents);
        andy->setAmount(11_cents);
        credit(account,
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
        debit(account, Transaction{456_cents, "gorilla",
                                   Date{2020, Month::January, 20}});
        credit(account, Transaction{789_cents, "chimpanzee",
                                    Date{2020, Month::June, 1}});
        assertEqual(result, 3_cents - 5_cents + 11_cents, observer.balance());
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertAccountName(result, persistentMemory, "joe");
        assertDebitsSaved(result, persistentMemory, {mike.get()});
        assertCreditsSaved(result, persistentMemory, {joe.get(), andy.get()});
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
      },
      "joe");
}

void showAfterRemoveShowsRemainingTransactions(
    testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        AccountObserverStub observer;
        account.attach(&observer);
        const auto mike{std::make_shared<TransactionRecordInMemory>()};
        const auto andy{std::make_shared<TransactionRecordInMemory>()};
        const auto joe{std::make_shared<TransactionRecordInMemory>()};
        const auto bob{std::make_shared<TransactionRecordInMemory>()};
        factory.add(mike);
        factory.add(andy);
        factory.add(joe);
        factory.add(bob);
        credit(account,
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
        debit(account, Transaction{456_cents, "gorilla",
                                   Date{2020, Month::January, 20}});
        credit(account, Transaction{111_cents, "orangutan",
                                    Date{2020, Month::March, 4}});
        debit(account,
              Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
        account.removeDebit(
            Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
        account.removeCredit(
            Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
        assertEqual(result, 123_cents - 789_cents, observer.balance());
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertAccountName(result, persistentMemory, "joe");
        assertDebitsSaved(result, persistentMemory, {bob.get()});
        assertCreditsSaved(result, persistentMemory, {mike.get()});
        // assertEqual(result,
        //             Transaction{123_cents, "ape", Date{2020, Month::June,
        //             2}}, mike->initializedTransaction());
        // assertEqual(
        //     result,
        //     Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
        //     bob->initializedTransaction());
      },
      "joe");
}

void showShowsVerifiedTransactions(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        AccountObserverStub observer;
        account.attach(&observer);
        const auto mike{std::make_shared<TransactionRecordInMemory>()};
        const auto andy{std::make_shared<TransactionRecordInMemory>()};
        const auto joe{std::make_shared<TransactionRecordInMemory>()};
        const auto bob{std::make_shared<TransactionRecordInMemory>()};
        factory.add(mike);
        factory.add(andy);
        factory.add(joe);
        factory.add(bob);
        credit(account,
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
        debit(account, Transaction{456_cents, "gorilla",
                                   Date{2020, Month::January, 20}});
        credit(account, Transaction{111_cents, "orangutan",
                                    Date{2020, Month::March, 4}});
        debit(account,
              Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
        account.verifyDebit(
            Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
        account.verifyCredit(
            Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
        assertEqual(result, 123_cents + 111_cents - 789_cents - 456_cents,
                    observer.balance());
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertAccountName(result, persistentMemory, "joe");
        assertDebitsSaved(result, persistentMemory, {andy.get(), bob.get()});
        assertCreditsSaved(result, persistentMemory, {mike.get(), joe.get()});
        // assertTrue(result, andy->verified());
        // assertTrue(result, joe->verified());
        // assertEqual(result,
        //             Transaction{123_cents, "ape", Date{2020, Month::June,
        //             2}}, mike->initializedTransaction());
        // assertEqual(
        //     result,
        //     Transaction{456_cents, "gorilla", Date{2020, Month::January,
        //     20}}, andy->initializedTransaction());
        // assertEqual(
        //     result,
        //     Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}},
        //     joe->initializedTransaction());
        // assertEqual(
        //     result,
        //     Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
        //     bob->initializedTransaction());
      },
      "joe");
}

void saveSavesAllTransactions(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        const auto mike{std::make_shared<TransactionRecordStub>()};
        const auto andy{std::make_shared<TransactionRecordStub>()};
        const auto joe{std::make_shared<TransactionRecordStub>()};
        factory.add(mike);
        factory.add(andy);
        factory.add(joe);
        credit(account,
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
        debit(account, Transaction{456_cents, "gorilla",
                                   Date{2020, Month::January, 20}});
        credit(account, Transaction{789_cents, "chimpanzee",
                                    Date{2020, Month::June, 1}});
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertEqual(result, "joe", persistentMemory.accountName());
        assertContainsDebit(result, persistentMemory, {andy.get()});
        assertContainsCredit(result, persistentMemory, {joe.get()});
        assertContainsCredit(result, persistentMemory, {mike.get()});
        assertEqual(result,
                    Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                    mike->initializedTransaction());
        assertEqual(
            result,
            Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
            andy->initializedTransaction());
        assertEqual(
            result,
            Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
            joe->initializedTransaction());
      },
      "joe");
}

void loadLoadsAllTransactions(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        PersistentAccountStub persistentMemory;
        account.load(persistentMemory);
        assertEqual(result, &account, persistentMemory.observer());
        const auto mike{std::make_shared<TransactionRecordStub>()};
        const auto andy{std::make_shared<TransactionRecordStub>()};
        const auto joe{std::make_shared<TransactionRecordStub>()};
        const auto bob{std::make_shared<TransactionRecordStub>()};
        factory.add(mike);
        factory.add(andy);
        factory.add(joe);
        factory.add(bob);
        TransactionRecordDeserializationStub abel;
        TransactionRecordDeserializationStub cain;
        TransactionRecordDeserializationStub mark;
        TransactionRecordDeserializationStub luke;
        account.notifyThatCreditIsReady(abel);
        account.notifyThatCreditIsReady(cain);
        account.notifyThatDebitIsReady(mark);
        account.notifyThatDebitIsReady(luke);
        assertEqual(result, mike.get(), abel.observer());
        assertEqual(result, andy.get(), cain.observer());
        assertEqual(result, joe.get(), mark.observer());
        assertEqual(result, bob.get(), luke.observer());
        account.save(persistentMemory);
        assertDebitsSaved(result, persistentMemory, {joe.get(), bob.get()});
        assertCreditsSaved(result, persistentMemory, {mike.get(), andy.get()});
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

void showAfterRemovingVerifiedTransactionShowsRemaining(
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

void showShowsDuplicateVerifiedTransactions(testcpplite::TestResult &result) {
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
    PersistentAccountStub persistentMemory;
    account.save(persistentMemory);
    assertDebitsSaved(result, persistentMemory, {mike.get(), andy.get()});
    assertTrue(result, john.verified());
    assertTrue(result, alex.verified());
  });
}

void notifiesObserverOfUpdatedBalanceAfterAddingOrRemovingTransactions(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &factory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto mike{std::make_shared<TransactionRecordInMemory>()};
    const auto andy{std::make_shared<TransactionRecordInMemory>()};
    factory.add(mike);
    factory.add(andy);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result, 123_cents, observer.balance());
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    assertEqual(result, 123_cents - 456_cents, observer.balance());
    account.removeCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result, -456_cents, observer.balance());
  });
}

void notifiesObserverOfNewCredit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto transactionRecord{std::make_shared<TransactionRecordStub>()};
    transactionRecordFactory.add(transactionRecord);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                transactionRecord->initializedTransaction());
    assertEqual(result, transactionRecord.get(),
                observer.newTransactionRecord());
  });
}

void notifiesObserverOfNewDebit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto transactionRecord{std::make_shared<TransactionRecordStub>()};
    transactionRecordFactory.add(transactionRecord);
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                transactionRecord->initializedTransaction());
    assertEqual(result, transactionRecord.get(),
                observer.newTransactionRecord());
  });
}

void verifiesCreditTransactionRecord(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    const auto transactionRecord{std::make_shared<TransactionRecordInMemory>()};
    transactionRecordFactory.add(transactionRecord);
    TransactionRecordObserverStub observer;
    transactionRecord->attach(&observer);
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void verifiesDebitTransactionRecord(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    const auto transactionRecord{std::make_shared<TransactionRecordInMemory>()};
    transactionRecordFactory.add(transactionRecord);
    TransactionRecordObserverStub observer;
    transactionRecord->attach(&observer);
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyDebit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.verified());
  });
}

void notifiesObserverOfRemovedDebit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    const auto transactionRecord{std::make_shared<TransactionRecordInMemory>()};
    transactionRecordFactory.add(transactionRecord);
    TransactionRecordObserverStub observer;
    transactionRecord->attach(&observer);
    account.debit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeDebit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}

void notifiesObserverOfRemovedCredit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    const auto transactionRecord{std::make_shared<TransactionRecordInMemory>()};
    transactionRecordFactory.add(transactionRecord);
    TransactionRecordObserverStub observer;
    transactionRecord->attach(&observer);
    account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, observer.removed());
  });
}

void notifiesObserverOfTransactionsLoaded(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account, TransactionRecordFactoryStub &factory) {
        AccountObserverStub observer;
        account.attach(&observer);
        TransactionRecordObserverStub sue;
        TransactionRecordObserverStub sally;
        const auto bob{std::make_shared<TransactionRecordInMemory>()};
        bob->attach(&sue);
        const auto jan{std::make_shared<TransactionRecordInMemory>()};
        jan->attach(&sally);
        factory.add(bob);
        factory.add(jan);
        TransactionRecordDeserializationStub abel;
        TransactionRecordDeserializationStub luke;
        account.notifyThatCreditIsReady(abel);
        account.notifyThatDebitIsReady(luke);
        // assertEqual(result,
        //             Transaction{123_cents, "ape", Date{2020, Month::June,
        //             2}}, observer.creditAdded());
        // assertTrue(result, transactionRecord->verified());
        // assertEqual(result, {456_cents, "gorilla", Date{2020, Month::January,
        // 20}},
        //             observer.debitAdded());
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
    // assertEqual(result,
    //             Transaction{2300_cents - 789_cents - 456_cents, "reduction",
    //                         Date{2021, Month::March, 13}},
    //             observer.creditAdded());
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
