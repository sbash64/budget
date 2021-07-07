#include "account.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"
#include <functional>
#include <map>
#include <sbash64/budget/account.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <utility>

namespace sbash64::budget::account {
namespace {
class TransactionRecordStub : public TransactionRecord {
public:
  void verify() override { verified_ = true; }

  [[nodiscard]] auto verified() const -> bool { return verified_; }

  void ready(const VerifiableTransaction &) override {}

private:
  bool verified_{};
};

class TransactionRecordFactoryStub : public TransactionRecord::Factory {
public:
  void add(std::shared_ptr<TransactionRecord> record, const Transaction &t) {
    transactionRecords[t] = std::move(record);
  }

  auto transaction() -> Transaction { return transaction_; }

  auto make(const Transaction &t)
      -> std::shared_ptr<TransactionRecord> override {
    transaction_ = t;
    return transactionRecords.count(t) == 0 ? nullptr
                                            : transactionRecords.at(t);
  }

private:
  std::map<Transaction, std::shared_ptr<TransactionRecord>> transactionRecords;
  Transaction transaction_;
};

class AccountObserverStub : public Account::Observer {
public:
  auto balance() -> USD { return balance_; }

  void notifyThatBalanceHasChanged(USD balance) override { balance_ = balance; }

  auto creditAdded() -> Transaction { return creditAdded_; }

  void notifyThatCreditHasBeenAdded(TransactionRecord &tr,
                                    const Transaction &t) override {
    creditAdded_ = t;
    newTransactionRecord_ = &tr;
  }

  auto newTransactionRecord() -> const TransactionRecord * {
    return newTransactionRecord_;
  }

  void notifyThatDebitHasBeenAdded(TransactionRecord &tr,
                                   const Transaction &t) override {
    debitAdded_ = t;
    newTransactionRecord_ = &tr;
  }

  auto debitAdded() -> Transaction { return debitAdded_; }

  auto debitRemoved() -> Transaction { return debitRemoved_; }

  auto debitsRemoved() -> Transactions { return debitsRemoved_; }

  void notifyThatDebitHasBeenRemoved(const Transaction &t) override {
    debitRemoved_ = t;
    debitsRemoved_.push_back(t);
  }

  auto creditRemoved() -> Transaction { return creditRemoved_; }

  void notifyThatCreditHasBeenRemoved(const Transaction &t) override {
    creditRemoved_ = t;
  }

private:
  TransactionRecord *newTransactionRecord_{};
  Transaction creditAdded_;
  Transaction debitAdded_;
  Transaction debitRemoved_;
  Transaction creditRemoved_;
  Transactions debitsRemoved_;
  USD balance_{};
};
} // namespace
constexpr auto to_integral(Transaction::Type e) ->
    typename std::underlying_type<Transaction::Type>::type {
  return static_cast<typename std::underlying_type<Transaction::Type>::type>(e);
}

static void assertContains(testcpplite::TestResult &result,
                           const VerifiableTransactions &transactions,
                           const VerifiableTransaction &transaction) {
  assertTrue(result, std::find(transactions.begin(), transactions.end(),
                               transaction) != transactions.end());
}

static void assertContainsCredit(testcpplite::TestResult &result,
                                 PersistentAccountStub &persistentMemory,
                                 const VerifiableTransaction &transaction) {
  assertContains(result, persistentMemory.credits(), transaction);
}

static void assertContainsDebit(testcpplite::TestResult &result,
                                PersistentAccountStub &persistentMemory,
                                const VerifiableTransaction &transaction) {
  assertContains(result, persistentMemory.debits(), transaction);
}

static void assertEqual(testcpplite::TestResult &result,
                        const VerifiableTransactions &expected,
                        const VerifiableTransactions &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (VerifiableTransactions::size_type i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

static void assertDebitsSaved(testcpplite::TestResult &result,
                              PersistentAccountStub &persistentMemory,
                              const VerifiableTransactions &transactions) {
  assertEqual(result, persistentMemory.debits(), transactions);
}

static void assertCreditsSaved(testcpplite::TestResult &result,
                               PersistentAccountStub &persistentMemory,
                               const VerifiableTransactions &transactions) {
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
      [&](InMemoryAccount &account) {
        AccountObserverStub observer;
        account.attach(&observer);
        credit(account,
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
        debit(account, Transaction{456_cents, "gorilla",
                                   Date{2020, Month::January, 20}});
        credit(account, Transaction{789_cents, "chimpanzee",
                                    Date{2020, Month::June, 1}});
        assertEqual(result, 123_cents - 456_cents + 789_cents,
                    observer.balance());
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertAccountName(result, persistentMemory, "joe");
        assertDebitsSaved(
            result, persistentMemory,
            {{Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
              false}});
        assertCreditsSaved(
            result, persistentMemory,
            {{Transaction{123_cents, "ape", Date{2020, Month::June, 2}}, false},
             {Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
              false}});
      },
      "joe");
}

void showAfterRemoveShowsRemainingTransactions(
    testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account) {
        AccountObserverStub observer;
        account.attach(&observer);
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
        assertDebitsSaved(
            result, persistentMemory,
            {{Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
              false}});
        assertCreditsSaved(
            result, persistentMemory,
            {{Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
              false}});
      },
      "joe");
}

void showShowsVerifiedTransactions(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account) {
        AccountObserverStub observer;
        account.attach(&observer);
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
        assertDebitsSaved(
            result, persistentMemory,
            {{Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
              true},
             {Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
              false}});
        assertCreditsSaved(
            result, persistentMemory,
            {{Transaction{123_cents, "ape", Date{2020, Month::June, 2}}, false},
             {Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}},
              true}});
      },
      "joe");
}

void saveSavesAllTransactions(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account) {
        credit(account,
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
        debit(account, Transaction{456_cents, "gorilla",
                                   Date{2020, Month::January, 20}});
        credit(account, Transaction{789_cents, "chimpanzee",
                                    Date{2020, Month::June, 1}});
        PersistentAccountStub persistentMemory;
        account.save(persistentMemory);
        assertEqual(result, "joe", persistentMemory.accountName());
        assertContainsDebit(
            result, persistentMemory,
            {{456_cents, "gorilla", Date{2020, Month::January, 20}}});
        assertContainsCredit(
            result, persistentMemory,
            {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}});
        assertContainsCredit(result, persistentMemory,
                             {{123_cents, "ape", Date{2020, Month::June, 2}}});
      },
      "joe");
}

void loadLoadsAllTransactions(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    PersistentAccountStub persistentMemory;
    account.load(persistentMemory);
    assertEqual(result, &account, persistentMemory.observer());
    account.notifyThatCreditHasBeenDeserialized(
        {{123_cents, "ape", Date{2020, Month::June, 2}}, false});
    account.notifyThatCreditHasBeenDeserialized(
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false});
    account.notifyThatDebitHasBeenDeserialized(
        {{456_cents, "gorilla", Date{2020, Month::January, 20}}, false});
    account.save(persistentMemory);
    assertDebitsSaved(
        result, persistentMemory,
        {{Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
          false}});
    assertCreditsSaved(
        result, persistentMemory,
        {{Transaction{123_cents, "ape", Date{2020, Month::June, 2}}, false},
         {Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
          false}});
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

void findUnverifiedDebitsReturnsUnverifiedDebitsMatchingAmount(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    debit(account,
          Transaction{123_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{456_cents, "orangutan", Date{2020, Month::March, 4}});
    debit(account,
          Transaction{123_cents, "chimpanzee", Date{2020, Month::June, 1}});
    account.verifyDebit(
        Transaction{123_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verifyDebit(
        Transaction{456_cents, "orangutan", Date{2020, Month::March, 4}});
    assertEqual(
        result,
        {Transaction{123_cents, "chimpanzee", Date{2020, Month::June, 1}},
         Transaction{123_cents, "ape", Date{2020, Month::June, 2}}},
        account.findUnverifiedDebits(123_cents));
  });
}

void findUnverifiedCreditsReturnsUnverifiedCreditsMatchingAmount(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    credit(account,
           Transaction{123_cents, "gorilla", Date{2020, Month::January, 20}});
    credit(account,
           Transaction{456_cents, "orangutan", Date{2020, Month::March, 4}});
    credit(account,
           Transaction{123_cents, "chimpanzee", Date{2020, Month::June, 1}});
    account.verifyCredit(
        Transaction{123_cents, "gorilla", Date{2020, Month::January, 20}});
    account.verifyCredit(
        Transaction{456_cents, "orangutan", Date{2020, Month::March, 4}});
    assertEqual(
        result,
        {Transaction{123_cents, "chimpanzee", Date{2020, Month::June, 1}},
         Transaction{123_cents, "ape", Date{2020, Month::June, 2}}},
        account.findUnverifiedCredits(123_cents));
  });
}

void showAfterRemovingVerifiedTransactionShowsRemaining(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
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
    assertDebitsSaved(
        result, persistentMemory,
        {{Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
          false}});
  });
}

void showShowsDuplicateVerifiedTransactions(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
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
    assertDebitsSaved(
        result, persistentMemory,
        {{Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
          true},
         {Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
          true}});
  });
}

void notifiesObserverOfUpdatedBalanceAfterAddingOrRemovingTransactions(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    AccountObserverStub observer;
    account.attach(&observer);
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
    transactionRecordFactory.add(
        transactionRecord,
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                observer.creditAdded());
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
    transactionRecordFactory.add(
        transactionRecord,
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                observer.debitAdded());
    assertEqual(result, transactionRecord.get(),
                observer.newTransactionRecord());
  });
}

void verifiesCreditTransactionRecord(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    const auto transactionRecord{std::make_shared<TransactionRecordStub>()};
    transactionRecordFactory.add(
        transactionRecord,
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, transactionRecord->verified());
  });
}

void verifiesDebitTransactionRecord(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    const auto transactionRecord{std::make_shared<TransactionRecordStub>()};
    transactionRecordFactory.add(
        transactionRecord,
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.verifyDebit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertTrue(result, transactionRecord->verified());
  });
}

void notifiesObserverOfRemovedDebit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    AccountObserverStub observer;
    account.attach(&observer);
    account.debit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeDebit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                observer.debitRemoved());
  });
}

void notifiesObserverOfRemovedCredit(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    AccountObserverStub observer;
    account.attach(&observer);
    account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.removeCredit(
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                observer.creditRemoved());
  });
}

void notifiesObserverOfTransactionsLoaded(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account,
                  TransactionRecordFactoryStub &transactionRecordFactory) {
    AccountObserverStub observer;
    account.attach(&observer);
    const auto transactionRecord{std::make_shared<TransactionRecordStub>()};
    transactionRecordFactory.add(
        transactionRecord,
        Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
    account.notifyThatCreditHasBeenDeserialized(
        {{123_cents, "ape", Date{2020, Month::June, 2}}, true});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                observer.creditAdded());
    assertTrue(result, transactionRecord->verified());
    account.notifyThatDebitHasBeenDeserialized(
        {{456_cents, "gorilla", Date{2020, Month::January, 20}}, false});
    assertEqual(result, {456_cents, "gorilla", Date{2020, Month::January, 20}},
                observer.debitAdded());
  });
}

void reduceReducesToOneTransaction(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    credit(account, Transaction{2300_cents, "orangutan",
                                Date{2020, Month::February, 2}});
    account.reduce(Date{2021, Month::March, 13});
    PersistentAccountStub persistentMemory;
    account.save(persistentMemory);
    assertCreditsSaved(result, persistentMemory,
                       {{Transaction{2300_cents - 789_cents - 456_cents,
                                     "reduction", Date{2021, Month::March, 13}},
                         true}});
  });
}

void notifiesObserverOfTransactionsWhenReducing(
    testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    credit(account, Transaction{2300_cents, "orangutan",
                                Date{2020, Month::February, 2}});
    AccountObserverStub observer;
    account.attach(&observer);
    account.reduce(Date{2021, Month::March, 13});
    assertEqual(
        result,
        {Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}},
         Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}},
        observer.debitsRemoved());
    assertEqual(
        result,
        Transaction{2300_cents, "orangutan", Date{2020, Month::February, 2}},
        observer.creditRemoved());
    assertEqual(result,
                Transaction{2300_cents - 789_cents - 456_cents, "reduction",
                            Date{2021, Month::March, 13}},
                observer.creditAdded());
  });
}

void returnsBalance(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
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
  testAccount([&](InMemoryAccount &account) {
    debit(account,
          Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
    debit(account,
          Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    credit(account,
           Transaction{300_cents, "orangutan", Date{2020, Month::February, 2}});
    account.reduce(Date{2021, Month::March, 13});
    PersistentAccountStub persistentMemory;
    account.save(persistentMemory);
    assertDebitsSaved(result, persistentMemory,
                      {{Transaction{789_cents + 456_cents - 300_cents,
                                    "reduction", Date{2021, Month::March, 13}},
                        true}});
  });
}
} // namespace sbash64::budget::account
