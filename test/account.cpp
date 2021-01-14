#include "account.hpp"
#include "persistent-memory-stub.hpp"
#include "sbash64/budget/budget.hpp"
#include "usd.hpp"
#include "view-stub.hpp"
#include <sbash64/budget/bank.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
constexpr auto to_integral(Transaction::Type e) ->
    typename std::underlying_type<Transaction::Type>::type {
  return static_cast<typename std::underlying_type<Transaction::Type>::type>(e);
}

static void assertEqual(testcpplite::TestResult &result,
                        const TransactionWithType &expected,
                        const TransactionWithType &actual) {
  assertEqual(result, expected.transaction, actual.transaction);
  assertEqual(result, to_integral(expected.type), to_integral(actual.type));
}

static void assertContains(testcpplite::TestResult &result,
                           const Transactions &transactions,
                           const Transaction &transaction) {
  assertTrue(result, std::find(transactions.begin(), transactions.end(),
                               transaction) != transactions.end());
}

static void assertContainsCredit(testcpplite::TestResult &result,
                                 PersistentMemoryStub &persistentMemory,
                                 const Transaction &transaction) {
  assertContains(result, persistentMemory.credits(), transaction);
}

static void assertContainsDebit(testcpplite::TestResult &result,
                                PersistentMemoryStub &persistentMemory,
                                const Transaction &transaction) {
  assertContains(result, persistentMemory.debits(), transaction);
}

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<TransactionWithType> &expected,
                        const std::vector<TransactionWithType> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<TransactionWithType>::size_type i{0}; i < expected.size();
       ++i)
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
  assertEqual(
      result,
      {debit(Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}}),
       credit(Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}),
       credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}})},
      view.accountTransactions());
}

void showAfterRemoveShowsRemainingTransactionsInChronologicalOrderAndBalance(
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
  assertEqual(
      result,
      {debit(Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}),
       credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}})},
      view.accountTransactions());
}

void saveSavesAllTransactions(testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
  credit(account, Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  debit(account,
        Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  credit(account,
         Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  PersistentMemoryStub persistentMemory;
  account.save(persistentMemory);
  assertEqual(result, "joe", persistentMemory.accountName());
  assertContainsDebit(
      result, persistentMemory,
      Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  assertContainsCredit(
      result, persistentMemory,
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  assertContainsCredit(
      result, persistentMemory,
      Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
}

void loadLoadsAllTransactions(testcpplite::TestResult &result) {
  PersistentMemoryStub persistentMemory;
  persistentMemory.setCreditsToLoad(
      {Transaction{123_cents, "ape", Date{2020, Month::June, 2}},
       Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}});
  persistentMemory.setDebitsToLoad(
      {Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}}});
  InMemoryAccount account{""};
  account.load(persistentMemory);
  ViewStub view;
  show(account, view);
  assertEqual(
      result,
      {debit(Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}}),
       credit(Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}),
       credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}})},
      view.accountTransactions());
}

void rename(testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
  account.rename("mike");
  ViewStub view;
  show(account, view);
  assertAccountName(result, view, "mike");
}
} // namespace sbash64::budget::account
