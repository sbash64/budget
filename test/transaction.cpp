#include "transaction.hpp"
#include "usd.hpp"

#include <functional>
#include <sbash64/budget/transaction.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::transaction {
namespace {
class TransactionSerializationStub : public TransactionSerialization {
public:
  void save(const ArchivableVerifiableTransaction &vt) override {
    archivableVerifiableTransaction_ = vt;
  }

  auto archivableVerifiableTransaction() -> ArchivableVerifiableTransaction {
    return archivableVerifiableTransaction_;
  }

private:
  ArchivableVerifiableTransaction archivableVerifiableTransaction_;
};

class TransactionObserverStub : public ObservableTransaction::Observer {
public:
  void notifyThatIsVerified() override { verified_ = true; }

  void notifyThatIs(const Transaction &t) override { transaction_ = t; }

  auto transaction() -> Transaction { return transaction_; }

  [[nodiscard]] auto verified() const -> bool { return verified_; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

  void notifyThatWillBeRemoved() override { removed_ = true; }

  [[nodiscard]] auto archived() const -> bool { return archived_; }

  void notifyThatIsArchived() override {
    ++timesArchived_;
    archived_ = true;
  }

  [[nodiscard]] auto timesArchived() const -> int { return timesArchived_; }

private:
  Transaction transaction_;
  int timesArchived_{};
  bool verified_{};
  bool removed_{};
  bool archived_{};
};

class TransactionDeserializationStub : public TransactionDeserialization {
public:
  auto load() -> ArchivableVerifiableTransaction override { return {}; }
};
} // namespace

static void testObservableTransactionInMemory(
    const std::function<void(ObservableTransaction &)> &f) {
  ObservableTransactionInMemory record;
  f(record);
}

void verifiesMatchingInitializedTransaction(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertTrue(result,
               record.verifies(Transaction{789_cents, "chimpanzee",
                                           Date{2020, Month::June, 1}}));
  });
}

void doesNotVerifyUnequalInitializedTransaction(
    testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertFalse(result, record.verifies(Transaction{
                            789_cents, "gorilla", Date{2020, Month::June, 1}}));
  });
}

void doesNotVerifyMatchingInitializedTransactionTwice(
    testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    record.verifies(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertFalse(result,
                record.verifies(Transaction{789_cents, "chimpanzee",
                                            Date{2020, Month::June, 1}}));
  });
}

void savesVerificationByQuery(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    record.verifies(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    TransactionSerializationStub serialization;
    record.save(serialization);
    assertTrue(result,
               serialization.archivableVerifiableTransaction().verified);
  });
}

void savesInitializedTransaction(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    TransactionSerializationStub serialization;
    record.save(serialization);
    assertEqual(
        result,
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
        serialization.archivableVerifiableTransaction());
  });
}

void savesArchival(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    TransactionSerializationStub serialization;
    record.archive();
    record.save(serialization);
    assertTrue(result,
               serialization.archivableVerifiableTransaction().archived);
  });
}

void notifiesObserverOfVerificationByQuery(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    TransactionObserverStub observer;
    record.attach(observer);
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    record.verifies(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertTrue(result, observer.verified());
  });
}

void notifiesObserverOfRemoval(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    TransactionObserverStub observer;
    record.attach(observer);
    record.remove();
    assertTrue(result, observer.removed());
  });
}

void notifiesObserverOfRemovalByQuery(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    TransactionObserverStub observer;
    record.attach(observer);
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    record.removes(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertTrue(result, observer.removed());
  });
}

void notifiesObserverOfArchival(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    TransactionObserverStub observer;
    record.attach(observer);
    record.archive();
    assertTrue(result, observer.archived());
  });
}

void doesNotNotifyObserverOfArchivalTwice(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    TransactionObserverStub observer;
    record.attach(observer);
    record.archive();
    record.archive();
    assertEqual(result, 1, observer.timesArchived());
  });
}

void notifiesObserverOfInitializedTransaction(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    TransactionObserverStub observer;
    record.attach(observer);
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertEqual(
        result,
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}},
        observer.transaction());
  });
}

void removesInitializedTransaction(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertTrue(result, record.removes(Transaction{789_cents, "chimpanzee",
                                                  Date{2020, Month::June, 1}}));
  });
}

void doesNotRemoveUnequalTransaction(testcpplite::TestResult &result) {
  testObservableTransactionInMemory([&result](ObservableTransaction &record) {
    record.initialize(
        Transaction{789_cents, "chimpanzee", Date{2020, Month::June, 1}});
    assertFalse(result,
                record.removes(Transaction{789_cents, "chimpanzee",
                                           Date{2021, Month::June, 1}}));
  });
}
} // namespace sbash64::budget::transaction
