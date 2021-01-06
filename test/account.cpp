#include "account.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"
#include "view-stub.hpp"
#include <sbash64/budget/account.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::account {
constexpr auto to_integral(Transaction::Type e) ->
    typename std::underlying_type<Transaction::Type>::type {
  return static_cast<typename std::underlying_type<Transaction::Type>::type>(e);
}

static void assertEqual(testcpplite::TestResult &result,
                        const PrintableTransaction &expected,
                        const PrintableTransaction &actual) {
  assertEqual(result, expected.transaction, actual.transaction);
  assertEqual(result, to_integral(expected.type), to_integral(actual.type));
}

static void assertContainsCredit(testcpplite::TestResult &result,
                                 PersistentMemoryStub &persistentMemory,
                                 const Transaction &transaction) {
  assertTrue(result, std::find(persistentMemory.credits().begin(),
                               persistentMemory.credits().end(), transaction) !=
                         persistentMemory.credits().end());
}

static void assertContainsDebit(testcpplite::TestResult &result,
                                PersistentMemoryStub &persistentMemory,
                                const Transaction &transaction) {
  assertTrue(result, std::find(persistentMemory.debits().begin(),
                               persistentMemory.debits().end(),
                               transaction) != persistentMemory.debits().end());
}

void showShowsAllTransactionsInChronologicalOrderAndBalance(
    testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
  account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  account.debit(
      Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  account.credit(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  ViewStub printer;
  account.show(printer);
  assertEqual(result, "joe", printer.accountName());
  assertEqual(result, 123_cents - 456_cents + 789_cents,
              printer.accountBalance());
  assertEqual(result,
              printableDebit(Transaction{456_cents, "gorilla",
                                         Date{2020, Month::January, 20}}),
              printer.accountTransactions().at(0));
  assertEqual(result,
              printableCredit(Transaction{789_cents, "chimpanzee",
                                          Date{2020, Month::June, 1}}),
              printer.accountTransactions().at(1));
  assertEqual(result,
              printableCredit(
                  Transaction{123_cents, "ape", Date{2020, Month::June, 2}}),
              printer.accountTransactions().at(2));
}

void saveSavesAllTransactions(testcpplite::TestResult &result) {
  InMemoryAccount account{"joe"};
  account.credit(Transaction{123_cents, "ape", Date{2020, Month::June, 2}});
  account.debit(
      Transaction{456_cents, "gorilla", Date{2020, Month::January, 20}});
  account.credit(
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
} // namespace sbash64::budget::account
