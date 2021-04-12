#include "account.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"
#include "view-stub.hpp"
#include <functional>
#include <sbash64/budget/bank.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <utility>

namespace sbash64::budget::account {
namespace {
class AccountObserverStub : public Account::Observer {
public:
  auto balance() -> USD { return balance_; }

  void notifyThatBalanceHasChanged(USD balance) override { balance_ = balance; }

  auto creditAdded() -> Transaction { return creditAdded_; }

  void notifyThatCreditHasBeenAdded(const Transaction &t) override {
    creditAdded_ = t;
  }

  void notifyThatDebitHasBeenAdded(const Transaction &t) override {
    debitAdded_ = t;
  }

  auto debitAdded() -> Transaction { return debitAdded_; }

  auto debitRemoved() -> Transaction { return debitRemoved_; }

  void notifyThatDebitHasBeenRemoved(const Transaction &t) override {
    debitRemoved_ = t;
  }

  auto creditRemoved() -> Transaction { return creditRemoved_; }

  void notifyThatCreditHasBeenRemoved(const Transaction &t) override {
    creditRemoved_ = t;
  }

private:
  Transaction creditAdded_;
  Transaction debitAdded_;
  Transaction debitRemoved_;
  Transaction creditRemoved_;
  USD balance_{};
};
} // namespace
constexpr auto to_integral(Transaction::Type e) ->
    typename std::underlying_type<Transaction::Type>::type {
  return static_cast<typename std::underlying_type<Transaction::Type>::type>(e);
}

static void assertEqual(testcpplite::TestResult &result,
                        const VerifiableTransactionWithType &expected,
                        const VerifiableTransactionWithType &actual) {
  assertEqual(result, expected.verifiableTransaction,
              actual.verifiableTransaction);
  assertEqual(result, to_integral(expected.type), to_integral(actual.type));
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

static void
assertEqual(testcpplite::TestResult &result,
            const std::vector<VerifiableTransactionWithType> &expected,
            const std::vector<VerifiableTransactionWithType> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<VerifiableTransactionWithType>::size_type i{0};
       i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

static void credit(Account &account, const Transaction &t) {
  account.credit(t);
}

static void debit(Account &account, const Transaction &t) { account.debit(t); }

static void show(Account &account, View &view) { account.show(view); }

static void assertAccountName(testcpplite::TestResult &result, ViewStub &view,
                              std::string_view expected) {
  assertEqual(result, std::string{expected}, view.accountName());
}

static void assertShown(testcpplite::TestResult &result, ViewStub &view,
                        const std::vector<VerifiableTransactionWithType> &t) {
  assertEqual(result, t, view.accountTransactions());
}

static void testAccount(const std::function<void(InMemoryAccount &)> &f,
                        std::string name = {}) {
  InMemoryAccount account{std::move(name)};
  f(account);
}

void showShowsAllTransactionsInChronologicalOrderAndBalance(
    testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account) {
        credit(account,
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
        debit(account, Transaction{456_cents, "gorilla",
                                   Date{2020, Month::January, 20}});
        credit(account, Transaction{789_cents, "chimpanzee",
                                    Date{2020, Month::June, 1}});
        ViewStub view;
        show(account, view);
        assertAccountName(result, view, "joe");
        assertEqual(result, 123_cents - 456_cents + 789_cents,
                    view.accountBalance());
        assertShown(result, view,
                    {debit(Transaction{456_cents, "gorilla",
                                       Date{2020, Month::January, 20}}),
                     credit(Transaction{789_cents, "chimpanzee",
                                        Date{2020, Month::June, 1}}),
                     credit(Transaction{123_cents, "ape",
                                        Date{2020, Month::June, 2}})});
      },
      "joe");
}

void showAfterRemoveShowsRemainingTransactions(
    testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account) {
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
        ViewStub view;
        show(account, view);
        assertAccountName(result, view, "joe");
        assertEqual(result, 123_cents - 789_cents, view.accountBalance());
        assertShown(result, view,
                    {debit(Transaction{789_cents, "chimpanzee",
                                       Date{2020, Month::June, 1}}),
                     credit(Transaction{123_cents, "ape",
                                        Date{2020, Month::June, 2}})});
      },
      "joe");
}

void showShowsVerifiedTransactions(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account) {
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
        ViewStub view;
        show(account, view);
        assertAccountName(result, view, "joe");
        assertEqual(result, 123_cents + 111_cents - 789_cents - 456_cents,
                    view.accountBalance());
        assertShown(result, view,
                    {verifiedDebit(Transaction{456_cents, "gorilla",
                                               Date{2020, Month::January, 20}}),
                     verifiedCredit(Transaction{111_cents, "orangutan",
                                                Date{2020, Month::March, 4}}),
                     debit(Transaction{789_cents, "chimpanzee",
                                       Date{2020, Month::June, 1}}),
                     credit(Transaction{123_cents, "ape",
                                        Date{2020, Month::June, 2}})});
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
    ViewStub view;
    show(account, view);
    assertShown(
        result, view,
        {debit(
             Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}}),
         credit(
             Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}),
         credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}})});
  });
}

void rename(testcpplite::TestResult &result) {
  testAccount(
      [&](InMemoryAccount &account) {
        account.rename("mike");
        ViewStub view;
        show(account, view);
        assertAccountName(result, view, "mike");
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
    ViewStub view;
    show(account, view);
    assertShown(result, view,
                {debit(Transaction{789_cents, "chimpanzee",
                                   Date{2020, Month::June, 1}})});
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
    ViewStub view;
    show(account, view);
    assertShown(result, view,
                {verifiedDebit(Transaction{456_cents, "gorilla",
                                           Date{2020, Month::January, 20}}),
                 verifiedDebit(Transaction{456_cents, "gorilla",
                                           Date{2020, Month::January, 20}})});
  });
}

void notifiesObserverOfUpdatedBalanceAfterAddingOrRemovingTransactions(
    testcpplite::TestResult &result) {
  InMemoryAccount account{""};
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
}

void notifiesObserverOfNewCredit(testcpplite::TestResult &result) {
  InMemoryAccount account{""};
  AccountObserverStub observer;
  account.attach(&observer);
  credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  assertEqual(result, Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
              observer.creditAdded());
}

void notifiesObserverOfNewDebit(testcpplite::TestResult &result) {
  InMemoryAccount account{""};
  AccountObserverStub observer;
  account.attach(&observer);
  debit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  assertEqual(result, Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
              observer.debitAdded());
}

void notifiesObserverOfRemovedDebit(testcpplite::TestResult &result) {
  InMemoryAccount account{""};
  AccountObserverStub observer;
  account.attach(&observer);
  account.debit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  account.removeDebit(
      Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  assertEqual(result, Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
              observer.debitRemoved());
}

void notifiesObserverOfRemovedCredit(testcpplite::TestResult &result) {
  InMemoryAccount account{""};
  AccountObserverStub observer;
  account.attach(&observer);
  account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  account.removeCredit(
      Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  assertEqual(result, Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
              observer.creditRemoved());
}

void notifiesObserverOfTransactionsLoaded(testcpplite::TestResult &result) {
  testAccount([&](InMemoryAccount &account) {
    AccountObserverStub observer;
    account.attach(&observer);
    account.notifyThatCreditHasBeenDeserialized(
        {{123_cents, "ape", Date{2020, Month::June, 2}}, true});
    assertEqual(result,
                Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
                observer.creditAdded());
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
    ViewStub view;
    show(account, view);
    assertShown(result, view,
                {verifiedCredit(Transaction{2300_cents - 789_cents - 456_cents,
                                            "reduction",
                                            Date{2021, Month::March, 13}})});
  });
}
} // namespace sbash64::budget::account
