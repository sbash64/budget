#include "account.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"
#include "view-stub.hpp"
#include <sbash64/budget/bank.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
namespace {
class AccountObserverStub : public Account::Observer {
public:
  auto balance() -> USD { return balance_; }

  void notifyThatBalanceHasChanged(USD balance) override { balance_ = balance; }

  auto creditAdded() -> Transaction { return creditAdded_; }

  void notifyThatCreditHasBeenAdded(const Transaction &t) { creditAdded_ = t; }

private:
  Transaction creditAdded_;
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

void showShowsAllTransactionsInChronologicalOrderAndBalance(
    testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
  credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  debit(account,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  credit(account,
         Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  ViewStub view;
  show(account, view);
  assertAccountName(result, view, "joe");
  assertEqual(result, 123_cents - 456_cents + 789_cents, view.accountBalance());
  assertShown(
      result, view,
      {debit(Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}}),
       credit(Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}),
       credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}})});
}

void showAfterRemoveShowsRemainingTransactions(
    testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
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
  ViewStub view;
  show(account, view);
  assertAccountName(result, view, "joe");
  assertEqual(result, 123_cents - 789_cents, view.accountBalance());
  assertShown(
      result, view,
      {debit(Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}),
       credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}})});
}

void showShowsVerifiedTransactions(testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
  credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  debit(account,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  credit(account,
         Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}});
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
  assertShown(
      result, view,
      {verifiedDebit(
           Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}}),
       verifiedCredit(
           Transaction{111_cents, "orangutan", Date{2020, Month::March, 4}}),
       debit(Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}),
       credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}})});
}

void saveSavesAllTransactions(testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
  credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  debit(account,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  credit(account,
         Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  PersistentAccountStub persistentMemory;
  account.save(persistentMemory);
  assertEqual(result, "joe", persistentMemory.accountName());
  assertContainsDebit(result, persistentMemory,
                      {{456_cents, "gorilla", Date{2020, Month::January, 20}}});
  assertContainsCredit(result, persistentMemory,
                       {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}});
  assertContainsCredit(result, persistentMemory,
                       {{123_cents, "ape", Date{2020, Month::June, 2}}});
}

void loadLoadsAllTransactions(testcpplite::TestResult &result) {
  PersistentAccountStub persistentMemory;
  persistentMemory.setCreditsToLoad(
      {{{123_cents, "ape", Date{2020, Month::June, 2}}},
       {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}}});
  persistentMemory.setDebitsToLoad(
      {{{456_cents, "gorilla", Date{2020, Month::January, 20}}}});
  InMemoryAccount account{""};
  account.load(persistentMemory);
  ViewStub view;
  show(account, view);
  assertShown(
      result, view,
      {debit(Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}}),
       credit(Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}),
       credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}})});
}

void rename(testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
  account.rename("mike");
  ViewStub view;
  show(account, view);
  assertAccountName(result, view, "mike");
}

void findUnverifiedDebitsReturnsUnverifiedDebitsMatchingAmount(
    testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
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
  assertEqual(result,
              {Transaction{123_cents, "chimpanzee", Date{2020, Month::June, 1}},
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}}},
              account.findUnverifiedDebits(123_cents));
}

void findUnverifiedCreditsReturnsUnverifiedCreditsMatchingAmount(
    testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
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
  assertEqual(result,
              {Transaction{123_cents, "chimpanzee", Date{2020, Month::June, 1}},
               Transaction{123_cents, "ape", Date{2020, Month::June, 2}}},
              account.findUnverifiedCredits(123_cents));
}

void showAfterRemovingVerifiedTransactionShowsRemaining(
    testcpplite::TestResult &result) {
  InMemoryAccount account{""};
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
}

void showShowsDuplicateVerifiedTransactions(testcpplite::TestResult &result) {
  InMemoryAccount account{""};
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
}

void notifiesObserverOfUpdatedBalanceAfterAddingTransactions(
    testcpplite::TestResult &result) {
  InMemoryAccount account{""};
  AccountObserverStub observer;
  account.attach(&observer);
  credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  assertEqual(result, 123_cents, observer.balance());
  debit(account,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  assertEqual(result, 123_cents - 456_cents, observer.balance());
}

void notifiesObserverOfNewCredit(testcpplite::TestResult &result) {
  InMemoryAccount account{""};
  AccountObserverStub observer;
  account.attach(&observer);
  credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  assertEqual(result, Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
              observer.creditAdded());
}
} // namespace sbash64::budget::account
