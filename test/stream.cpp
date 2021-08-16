#include "stream.hpp"
#include "usd.hpp"

#include <sbash64/budget/serialization.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sbash64::budget::streams {
namespace {
class BudgetDeserializationObserverStub
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

class SavesNameAccount : public SerializableAccount {
public:
  SavesNameAccount(std::ostream &stream, std::string name)
      : name{std::move(name)}, stream{stream} {}
  void load(AccountDeserialization &) override {}
  void save(AccountSerialization &) override { stream << name; }

private:
  std::string name;
  std::ostream &stream;
};

class AccountSerializationStub : public AccountSerialization {
public:
  void save(std::string_view name, USD funds,
            const std::vector<SerializableTransaction *> &credits,
            const std::vector<SerializableTransaction *> &debits) override {}
};

class AccountDeserializationStub : public AccountDeserialization {
public:
  void load(Observer &) override {}
};

class AccountToStreamFactoryStub : public AccountToStreamFactory {
public:
  auto make(std::ostream &) -> std::shared_ptr<AccountSerialization> override {
    return std::make_shared<AccountSerializationStub>();
  }
};

class AccountFromStreamFactoryStub : public AccountFromStreamFactory {
public:
  auto make(std::istream &)
      -> std::shared_ptr<AccountDeserialization> override {
    return std::make_shared<AccountDeserializationStub>();
  }
};

class TransactionDeserializationStub : public TransactionDeserialization {
public:
  void load(Observer &) override {}
};

class TransactionFromStreamFactoryStub : public TransactionFromStreamFactory {
public:
  auto make(std::istream &)
      -> std::shared_ptr<TransactionDeserialization> override {
    return std::make_shared<TransactionDeserializationStub>();
  }
};

class TransactionDeserializationObserverStub
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

  void notifyThatIsReady(TransactionDeserialization &) override {
    ReadsTransactionFromStream reads{stream};
    TransactionDeserializationObserverStub observer;
    observer.setOnReady(
        [&](const VerifiableTransaction &t) { transactions_.push_back(t); });
    reads.load(observer);
  }

  auto transactions() -> std::vector<VerifiableTransaction> {
    return transactions_;
  }

  auto funds() -> USD { return funds_; }

  void notifyThatFundsAreReady(USD usd) override { funds_ = usd; }

private:
  std::vector<VerifiableTransaction> transactions_;
  USD funds_{};
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

class TransactionToStreamFactoryStub : public TransactionToStreamFactory {
public:
  auto make(std::ostream &)
      -> std::shared_ptr<TransactionSerialization> override {
    return {};
  }
};

class SavesNameTransaction : public SerializableTransaction {
public:
  SavesNameTransaction(std::ostream &stream, std::string name)
      : name{std::move(name)}, stream{stream} {}
  void save(TransactionSerialization &) override { stream << name; }
  void load(TransactionDeserialization &) override {}

private:
  std::string name;
  std::ostream &stream;
};
} // namespace

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<VerifiableTransaction> &expected,
                        const std::vector<VerifiableTransaction> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<VerifiableTransaction>::size_type i{0}; i < expected.size();
       ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

void fromTransaction(testcpplite::TestResult &result) {
  std::stringstream stream;
  WritesTransactionToStream writesTransaction{stream};
  writesTransaction.save(
      {{324_cents, "hyvee", Date{2020, Month::February, 8}}, false});
  assertEqual(result, "3.24 hyvee 2/8/2020", stream.str());
}

void fromVerifiedTransaction(testcpplite::TestResult &result) {
  std::stringstream stream;
  WritesTransactionToStream writesTransaction{stream};
  writesTransaction.save(
      {{20_cents, "hyvee", Date{2020, Month::February, 8}}, true});
  assertEqual(result, "^0.20 hyvee 2/8/2020", stream.str());
}

void toTransaction(testcpplite::TestResult &result) {
  std::stringstream input{"3.24 hyvee 2/8/2020"};
  ReadsTransactionFromStream readsTransaction{input};
  TransactionDeserializationObserverStub observer;
  readsTransaction.load(observer);
  assertEqual(result,
              {{324_cents, "hyvee", Date{2020, Month::February, 8}}, false},
              observer.transaction());
}

void toVerifiedTransaction(testcpplite::TestResult &result) {
  std::stringstream input{"^3.24 hyvee 2/8/2020"};
  ReadsTransactionFromStream readsTransaction{input};
  TransactionDeserializationObserverStub observer;
  readsTransaction.load(observer);
  assertEqual(result,
              {{324_cents, "hyvee", Date{2020, Month::February, 8}}, true},
              observer.transaction());
}

void fromAccount(testcpplite::TestResult &result) {
  std::stringstream stream;
  TransactionToStreamFactoryStub factory;
  WritesAccountToStream accountSerialization{stream, factory};
  SavesNameTransaction steve{stream, "steve"};
  SavesNameTransaction sue{stream, "sue"};
  SavesNameTransaction allen{stream, "allen"};
  SavesNameTransaction john{stream, "john"};
  accountSerialization.save("jeff", 1234_cents, {&steve, &sue},
                            {&allen, &john});
  assertEqual(result, R"(jeff
funds 12.34
credits
steve
sue
debits
allen
john)",
              stream.str());
}

void nonfinalToAccount(testcpplite::TestResult &result) {
  std::stringstream input{
      R"(credits
debits
^27.34 hyvee 1/12/2021
9.87 walmart 6/15/2021
3.24 hyvee 2/8/2020

bobby)"};
  TransactionFromStreamFactoryStub factory;
  ReadsAccountFromStream readsAccount{input, factory};
  AccountDeserializationObserverStub observer{input};
  readsAccount.load(observer);
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}, true},
               {{987_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2020, Month::February, 8}}}},
              observer.transactions());
}

void finalToAccount(testcpplite::TestResult &result) {
  std::stringstream input{
      R"(credits
50 transfer from master 1/10/2021
25 transfer from master 4/12/2021
13.80 transfer from master 2/8/2021
debits)"};
  TransactionFromStreamFactoryStub factory;
  ReadsAccountFromStream accountDeserialization{input, factory};
  AccountDeserializationObserverStub observer{input};
  accountDeserialization.load(observer);
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::April, 12}}},
       {{1380_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      observer.transactions());
}

void toAccountWithFunds(testcpplite::TestResult &result) {
  std::stringstream input{
      R"(funds 12.32
credits
50 transfer from master 1/10/2021
25 transfer from master 4/12/2021
13.80 transfer from master 2/8/2021
debits
^27.34 hyvee 1/12/2021
9.87 walmart 6/15/2021
3.24 hyvee 2/8/2020)"};
  TransactionFromStreamFactoryStub factory;
  ReadsAccountFromStream accountDeserialization{input, factory};
  AccountDeserializationObserverStub observer{input};
  accountDeserialization.load(observer);
  assertEqual(result, 1232_cents, observer.funds());
}

void fromBudget(testcpplite::TestResult &result) {
  const auto stream{std::make_shared<std::stringstream>()};
  IoStreamFactoryStub streamFactory{stream};
  AccountToStreamFactoryStub accountSerializationFactory;
  WritesBudgetToStream sessionSerialization{streamFactory,
                                            accountSerializationFactory};
  SavesNameAccount jeff{*stream, "jeff"};
  SavesNameAccount steve{*stream, "steve"};
  SavesNameAccount sue{*stream, "sue"};
  SavesNameAccount allen{*stream, "allen"};
  sessionSerialization.save(jeff, {&steve, &sue, &allen});
  assertEqual(result, R"(
jeff

steve

sue

allen
)",
              '\n' + stream->str());
}

void toBudget(testcpplite::TestResult &result) {
  const auto input{std::make_shared<std::stringstream>(
      R"(jeff
steve
sue
allen)")};
  IoStreamFactoryStub streamFactory{input};
  AccountFromStreamFactoryStub accountDeserializationFactory;
  ReadsBudgetFromStream readsBudget{streamFactory,
                                    accountDeserializationFactory};
  BudgetDeserializationObserverStub observer;
  readsBudget.load(observer);
  assertEqual(result, "jeff", observer.primaryAccountName());
  assertEqual(result, "steve", observer.secondaryAccountNames().at(0));
  assertEqual(result, "sue", observer.secondaryAccountNames().at(1));
  assertEqual(result, "allen", observer.secondaryAccountNames().at(2));
}
} // namespace sbash64::budget::streams
