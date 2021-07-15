#include "stream.hpp"
#include "sbash64/budget/budget.hpp"
#include "usd.hpp"
#include <sbash64/budget/serialization.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sbash64::budget::stream {
namespace {
class AccountSerializationStub : public AccountSerialization {
public:
  void save(std::string_view name,
            const std::vector<ObservableTransaction *> &credits,
            const std::vector<ObservableTransaction *> &debits) override {}
};

class AccountDeserializationStub : public AccountDeserialization {
public:
  void load(Observer &) override {}
};

class TransactionRecordDeserializationStub : public TransactionDeserialization {
public:
  void load(Observer &) override {}
};

class StreamAccountSerializationFactoryStub : public AccountToStreamFactory {
public:
  auto make(std::ostream &) -> std::shared_ptr<AccountSerialization> override {
    return std::make_shared<AccountSerializationStub>();
  }
};

class StreamAccountDeserializationFactoryStub
    : public AccountFromStreamFactory {
public:
  auto make(std::istream &)
      -> std::shared_ptr<AccountDeserialization> override {
    return std::make_shared<AccountDeserializationStub>();
  }
};

class StreamTransactionRecordDeserializationFactoryStub
    : public ObservableTransactionFromStreamFactory {
public:
  auto make(std::istream &)
      -> std::shared_ptr<TransactionDeserialization> override {
    return std::make_shared<TransactionRecordDeserializationStub>();
  }
};

class SessionDeserializationObserverStub
    : public BudgetDeserialization::Observer {
public:
  void notifyThatPrimaryAccountIsReady(AccountDeserialization &,
                                       std::string_view name) override {
    primaryAccountName_ = name;
  }

  void notifyThatSecondaryAccountIsReady(AccountDeserialization &,
                                         std::string_view name) override {
    secondaryAccountNames_.emplace_back(name);
  }

  auto primaryAccountName() -> std::string { return primaryAccountName_; }

  auto secondaryAccountNames() -> std::vector<std::string> {
    return secondaryAccountNames_;
  }

private:
  std::string primaryAccountName_;
  std::vector<std::string> secondaryAccountNames_;
};

class TransactionRecordDeserializationObserverStub
    : public TransactionDeserialization::Observer {
public:
  auto transaction() -> VerifiableTransaction { return transaction_; }

  void ready(const VerifiableTransaction &t) override {
    transaction_ = t;
    if (onReady)
      onReady(t);
  }

  void setOnReady(std::function<void(const VerifiableTransaction &)> f) {
    onReady = std::move(f);
  }

private:
  VerifiableTransaction transaction_;
  std::function<void(const VerifiableTransaction &)> onReady;
};

class AccountDeserializationObserverStub
    : public AccountDeserialization::Observer {
public:
  explicit AccountDeserializationObserverStub(std::istream &stream)
      : stream{stream} {}

  void notifyThatCreditIsReady(TransactionDeserialization &) override {
    ReadsTransactionFromStream reads{stream};
    TransactionRecordDeserializationObserverStub observer;
    observer.setOnReady(
        [&](const VerifiableTransaction &t) { credits_.push_back(t); });
    reads.load(observer);
  }

  void notifyThatDebitIsReady(TransactionDeserialization &) override {
    ReadsTransactionFromStream reads{stream};
    TransactionRecordDeserializationObserverStub observer;
    observer.setOnReady(
        [&](const VerifiableTransaction &t) { debits_.push_back(t); });
    reads.load(observer);
  }

  auto credits() -> std::vector<VerifiableTransaction> { return credits_; }

  auto debits() -> std::vector<VerifiableTransaction> { return debits_; }

private:
  std::vector<VerifiableTransaction> credits_;
  std::vector<VerifiableTransaction> debits_;
  std::istream &stream;
};

class IoStreamFactoryStub : public IoStreamFactory {
public:
  explicit IoStreamFactoryStub(std::shared_ptr<std::iostream> stream)
      : stream{std::move(stream)} {}

  auto makeInput() -> std::shared_ptr<std::istream> override { return stream; }

  auto makeOutput() -> std::shared_ptr<std::ostream> override { return stream; }

private:
  std::shared_ptr<std::iostream> stream;
};

class SavesNameAccountStub : public Account {
public:
  SavesNameAccountStub(std::ostream &stream, std::string name)
      : name{std::move(name)}, stream{stream} {}
  void attach(Observer *) override {}
  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void removeCredit(const Transaction &) override {}
  void removeDebit(const Transaction &) override {}
  void load(AccountDeserialization &) override {}
  void rename(std::string_view) override {}
  void verifyDebit(const Transaction &) override {}
  void verifyCredit(const Transaction &) override {}
  void notifyThatCreditIsReady(TransactionDeserialization &) override {}
  void notifyThatDebitIsReady(TransactionDeserialization &) override {}
  void reduce(const Date &) override {}
  auto balance() -> USD override { return {}; }
  void remove() override {}

  void save(AccountSerialization &) override { stream << name; }

private:
  std::string name;
  std::ostream &stream;
};

class StreamTransactionRecordSerializationFactoryStub
    : public ObservableTransactionToStreamFactory {
public:
  auto make(std::ostream &)
      -> std::shared_ptr<TransactionSerialization> override {
    return {};
  }
};

class SavesNameTransactionRecordStub : public ObservableTransaction {
public:
  SavesNameTransactionRecordStub(std::ostream &stream, std::string name)
      : name{std::move(name)}, stream{stream} {}
  void attach(Observer *) override {}
  void initialize(const Transaction &) override {}
  void verify() override {}
  auto verifies(const Transaction &) -> bool override { return {}; }
  auto removes(const Transaction &) -> bool override { return {}; }
  void save(TransactionSerialization &) override { stream << name; }
  void load(TransactionDeserialization &) override {}
  void ready(const VerifiableTransaction &) override {}
  auto amount() -> USD override { return {}; }
  void remove() override {}

private:
  std::string name;
  std::ostream &stream;
};
} // namespace

void fromAccounts(testcpplite::TestResult &result) {
  const auto stream{std::make_shared<std::stringstream>()};
  StreamTransactionRecordSerializationFactoryStub factory;
  WritesAccountToStream accountSerialization{*stream, factory};
  SavesNameTransactionRecordStub steve{*stream, "steve"};
  SavesNameTransactionRecordStub sue{*stream, "sue"};
  SavesNameTransactionRecordStub allen{*stream, "allen"};
  SavesNameTransactionRecordStub john{*stream, "john"};
  accountSerialization.save("jeff", {&steve, &sue}, {&allen, &john});
  assertEqual(result, R"(
jeff
credits
steve
sue
debits
allen
john
)",
              '\n' + stream->str() + '\n');
}

void fromSession(testcpplite::TestResult &result) {
  const auto stream{std::make_shared<std::stringstream>()};
  IoStreamFactoryStub streamFactory{stream};
  StreamAccountSerializationFactoryStub accountSerializationFactory;
  WritesBudgetToStream sessionSerialization{streamFactory,
                                            accountSerializationFactory};
  SavesNameAccountStub jeff{*stream, "jeff"};
  SavesNameAccountStub steve{*stream, "steve"};
  SavesNameAccountStub sue{*stream, "sue"};
  SavesNameAccountStub allen{*stream, "allen"};
  sessionSerialization.save(jeff, {&steve, &sue, &allen});
  assertEqual(result, R"(
jeff

steve

sue

allen
)",
              '\n' + stream->str());
}

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<VerifiableTransaction> &expected,
                        const std::vector<VerifiableTransaction> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<VerifiableTransaction>::size_type i{0}; i < expected.size();
       ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

void toTransactionRecord(testcpplite::TestResult &result) {
  const auto input{std::make_shared<std::stringstream>("^3.24 hyvee 2/8/2020")};
  ReadsTransactionFromStream transactionRecordDeserialization{*input};
  TransactionRecordDeserializationObserverStub observer;
  transactionRecordDeserialization.load(observer);
  assertEqual(result,
              {{324_cents, "hyvee", Date{2020, Month::February, 8}}, true},
              observer.transaction());
}

void toAccounts(testcpplite::TestResult &result) {
  const auto input{std::make_shared<std::stringstream>(
      R"(credits
50 transfer from master 1/10/2021
25 transfer from master 4/12/2021
13.80 transfer from master 2/8/2021
debits
^27.34 hyvee 1/12/2021
9.87 walmart 6/15/2021
3.24 hyvee 2/8/2020

bobby)")};
  StreamTransactionRecordDeserializationFactoryStub factory;
  ReadsAccountFromStream accountDeserialization{*input, factory};
  AccountDeserializationObserverStub observer{*input};
  accountDeserialization.load(observer);
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::April, 12}}},
       {{1380_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      observer.credits());
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}, true},
               {{987_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2020, Month::February, 8}}}},
              observer.debits());
}

void toAccounts2(testcpplite::TestResult &result) {
  const auto input{std::make_shared<std::stringstream>(
      R"(credits
50 transfer from master 1/10/2021
25 transfer from master 4/12/2021
13.80 transfer from master 2/8/2021
debits
^27.34 hyvee 1/12/2021
9.87 walmart 6/15/2021
3.24 hyvee 2/8/2020)")};
  StreamTransactionRecordDeserializationFactoryStub factory;
  ReadsAccountFromStream accountDeserialization{*input, factory};
  AccountDeserializationObserverStub observer{*input};
  accountDeserialization.load(observer);
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::April, 12}}},
       {{1380_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      observer.credits());
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}, true},
               {{987_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2020, Month::February, 8}}}},
              observer.debits());
}

void toSession(testcpplite::TestResult &result) {
  const auto input{
      std::make_shared<std::stringstream>("jeff\nsteve\nsue\nallen")};
  IoStreamFactoryStub streamFactory{input};
  StreamAccountDeserializationFactoryStub accountDeserializationFactory;
  ReadsBudgetFromStream deserialization{streamFactory,
                                        accountDeserializationFactory};
  SessionDeserializationObserverStub observer;
  deserialization.load(observer);
  assertEqual(result, "jeff", observer.primaryAccountName());
  assertEqual(result, "steve", observer.secondaryAccountNames().at(0));
  assertEqual(result, "sue", observer.secondaryAccountNames().at(1));
  assertEqual(result, "allen", observer.secondaryAccountNames().at(2));
}
} // namespace sbash64::budget::stream
