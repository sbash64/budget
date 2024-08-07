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
  void notifyThatIncomeAccountIsReady(AccountDeserialization &) override {}

  void notifyThatExpenseAccountIsReady(AccountDeserialization &,
                                       std::string_view name) override {
    expenseAccountNames_.emplace_back(name);
  }

  auto expenseAccountNames() -> std::vector<std::string> {
    return expenseAccountNames_;
  }

  auto expenseAccountAllocations() -> std::vector<USD> {
    return expenseAccountAllocations_;
  }

  auto unallocatedIncome() -> USD { return unallocatedIncome_; }

private:
  std::vector<std::string> expenseAccountNames_;
  std::vector<USD> expenseAccountAllocations_;
  USD unallocatedIncome_;
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
  void save(const std::vector<SerializableTransaction *> &, USD) override {}
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
  auto load() -> ArchivableVerifiableTransaction override { return {}; }
};

class TransactionFromStreamFactoryStub : public TransactionFromStreamFactory {
public:
  auto make(std::istream &)
      -> std::shared_ptr<TransactionDeserialization> override {
    return std::make_shared<TransactionDeserializationStub>();
  }
};

class AccountDeserializationObserverStub
    : public AccountDeserialization::Observer {
public:
  explicit AccountDeserializationObserverStub(std::istream &stream)
      : stream{stream} {}

  void notifyThatIsReady(TransactionDeserialization &) override {
    ReadsTransactionFromStream reads{stream};
    transactions_.push_back(reads.load());
  }

  auto transactions() -> std::vector<ArchivableVerifiableTransaction> {
    return transactions_;
  }

  void notifyThatAllocatedIsReady(USD usd) override { allocation_ = usd; }

  auto allocation() -> USD { return allocation_; }

private:
  std::vector<ArchivableVerifiableTransaction> transactions_;
  std::istream &stream;
  USD allocation_{};
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

private:
  std::string name;
  std::ostream &stream;
};
} // namespace

static void
assertEqual(testcpplite::TestResult &result,
            const std::vector<ArchivableVerifiableTransaction> &expected,
            const std::vector<ArchivableVerifiableTransaction> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<ArchivableVerifiableTransaction>::size_type i{0};
       i < expected.size(); ++i)
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

void fromArchivedVerifiedTransaction(testcpplite::TestResult &result) {
  std::stringstream stream;
  WritesTransactionToStream writesTransaction{stream};
  writesTransaction.save(
      {{20_cents, "hyvee", Date{2020, Month::February, 8}}, true, true});
  assertEqual(result, "%0.20 hyvee 2/8/2020", stream.str());
}

void toTransaction(testcpplite::TestResult &result) {
  std::stringstream input{"3.24 hyvee 2/8/2020"};
  ReadsTransactionFromStream readsTransaction{input};
  assertEqual(
      result,
      {{324_cents, "hyvee", Date{2020, Month::February, 8}}, false, false},
      readsTransaction.load());
}

void toVerifiedTransaction(testcpplite::TestResult &result) {
  std::stringstream input{"^3.24 hyvee 2/8/2020"};
  ReadsTransactionFromStream readsTransaction{input};
  assertEqual(
      result,
      {{324_cents, "hyvee", Date{2020, Month::February, 8}}, true, false},
      readsTransaction.load());
}

void toArchivedVerifiedTransaction(testcpplite::TestResult &result) {
  std::stringstream input{"%3.24 hyvee 2/8/2020"};
  ReadsTransactionFromStream readsTransaction{input};
  assertEqual(
      result,
      {{324_cents, "hyvee", Date{2020, Month::February, 8}}, true, true},
      readsTransaction.load());
}

void fromAccount(testcpplite::TestResult &result) {
  std::stringstream stream;
  TransactionToStreamFactoryStub factory;
  WritesAccountToStream accountSerialization{stream, factory};
  SavesNameTransaction steve{stream, "steve"};
  SavesNameTransaction sue{stream, "sue"};
  SavesNameTransaction allen{stream, "allen"};
  SavesNameTransaction john{stream, "john"};
  accountSerialization.save({&steve, &sue, &allen, &john}, 123_cents);
  assertEqual(result, R"(1.23
steve
sue
allen
john
)",
              stream.str());
}

void nonfinalToAccount(testcpplite::TestResult &result) {
  std::stringstream input{
      R"(12.34
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
  assertEqual(result, 1234_cents, observer.allocation());
}

void finalToAccount(testcpplite::TestResult &result) {
  std::stringstream input{
      R"(12.34
50 transfer from master 1/10/2021
25 transfer from master 4/12/2021
13.80 transfer from master 2/8/2021)"};
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
  sessionSerialization.save(
      &jeff, {{&steve, "stevey"}, {&sue, "suzie yo"}, {&allen, "arod"}});
  assertEqual(result, R"(jeff
stevey
steve
suzie yo
sue
arod
allen)",
              stream->str());
}

void toBudget(testcpplite::TestResult &result) {
  const auto input{std::make_shared<std::stringstream>(
      R"(steve
sue is here
allen)")};
  IoStreamFactoryStub streamFactory{input};
  AccountFromStreamFactoryStub accountDeserializationFactory;
  ReadsBudgetFromStream readsBudget{streamFactory,
                                    accountDeserializationFactory};
  BudgetDeserializationObserverStub observer;
  readsBudget.load(observer);
  assertEqual(result, "steve", observer.expenseAccountNames().at(0));
  assertEqual(result, "sue is here", observer.expenseAccountNames().at(1));
  assertEqual(result, "allen", observer.expenseAccountNames().at(2));
}
} // namespace sbash64::budget::streams
