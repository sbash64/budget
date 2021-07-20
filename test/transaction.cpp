#include "transaction.hpp"
#include "usd.hpp"

#include <sbash64/budget/account.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::transaction {
namespace {
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

class TransactionDeserializationStub : public TransactionDeserialization {
public:
  void load(Observer &a) override { observer_ = &a; }

  auto observer() -> Observer * { return observer_; }

private:
  Observer *observer_{};
};
} // namespace

void verifies(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  record.initialize(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  assertTrue(result, record.verifies(Transaction{789_cents, "chimpanzee",
                                                 Date{2020, Month::June, 1}}));
}

void doesNotVerifyTwice(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  record.initialize(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  record.verifies(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  assertFalse(result, record.verifies(Transaction{789_cents, "chimpanzee",
                                                  Date{2020, Month::June, 1}}));
}

void notifiesObserverOfVerification(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionObserverStub observer;
  record.attach(&observer);
  record.verify();
  assertTrue(result, observer.verified());
}

void notifiesObserverOfVerificationByQuery(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionObserverStub observer;
  record.attach(&observer);
  record.initialize(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  record.verifies(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  assertTrue(result, observer.verified());
}

void savesVerification(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  record.verify();
  TransactionSerializationStub serialization;
  record.save(serialization);
  assertTrue(result, serialization.verifiableTransaction().verified);
}

void notifiesObserverOfRemoval(testcpplite::TestResult &result) {
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

void notifiesObserverOfLoadedValue(testcpplite::TestResult &result) {
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

void notifiesObserverOfInitializedValue(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionObserverStub observer;
  record.attach(&observer);
  record.initialize(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  assertEqual(result,
              Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
              observer.transaction());
}

void savesLoadedValue(testcpplite::TestResult &result) {
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

void removesLoadedValue(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  record.ready(
      {Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, true});
  assertTrue(result, record.removes(Transaction{789_cents, "chimpanzee",
                                                Date{2020, Month::June, 1}}));
}

void doesNotRemoveUnequalValue(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  record.ready(
      {Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, true});
  assertFalse(result, record.removes(Transaction{789_cents, "chimpanzee",
                                                 Date{2021, Month::June, 1}}));
}

void notifiesObserverOfRemovalByQuery(testcpplite::TestResult &result) {
  ObservableTransactionInMemory record;
  TransactionObserverStub observer;
  record.attach(&observer);
  record.initialize(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  record.removes(
      Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
  assertTrue(result, observer.removed());
}
} // namespace sbash64::budget::transaction
