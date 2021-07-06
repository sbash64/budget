#include "stream.hpp"
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
  void save(std::string_view name, const VerifiableTransactions &credits,
            const VerifiableTransactions &debits) override {}
};

class AccountDeserializationStub : public AccountDeserialization {
public:
  void load(Observer &) override {}
};

class StreamAccountSerializationFactoryStub
    : public StreamAccountSerializationFactory {
public:
  auto make(std::ostream &) -> std::shared_ptr<AccountSerialization> override {
    return std::make_shared<AccountSerializationStub>();
  }
};

class StreamAccountDeserializationFactoryStub
    : public StreamAccountDeserializationFactory {
public:
  auto make(std::istream &)
      -> std::shared_ptr<AccountDeserialization> override {
    return std::make_shared<AccountDeserializationStub>();
  }
};

class SessionDeserializationObserverStub
    : public SessionDeserialization::Observer {
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

class AccountDeserializationObserverStub
    : public AccountDeserialization::Observer {
public:
  void
  notifyThatDebitHasBeenDeserialized(const VerifiableTransaction &t) override {
    debits_.push_back(t);
  }

  void
  notifyThatCreditHasBeenDeserialized(const VerifiableTransaction &t) override {
    credits_.push_back(t);
  }

  auto credits() -> VerifiableTransactions { return credits_; }

  auto debits() -> VerifiableTransactions { return debits_; }

private:
  VerifiableTransactions credits_;
  VerifiableTransactions debits_;
};

class TransactionRecordDeserializationObserverStub
    : public TransactionRecordDeserialization::Observer {
public:
  auto transaction() -> VerifiableTransaction { return transaction_; }

  void notify(const VerifiableTransaction &t) { transaction_ = t; }

private:
  VerifiableTransaction transaction_;
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
  void show(View &) override {}
  void load(AccountDeserialization &) override {}
  void rename(std::string_view) override {}
  auto findUnverifiedDebits(USD) -> Transactions override { return {}; }
  auto findUnverifiedCredits(USD) -> Transactions override { return {}; }
  void verifyDebit(const Transaction &) override {}
  void verifyCredit(const Transaction &) override {}
  void
  notifyThatDebitHasBeenDeserialized(const VerifiableTransaction &) override {}
  void
  notifyThatCreditHasBeenDeserialized(const VerifiableTransaction &) override {}
  void reduce(const Date &) override {}
  auto balance() -> USD override { return {}; }

  void save(AccountSerialization &) override { stream << name; }

private:
  std::string name;
  std::ostream &stream;
};
} // namespace

void fromAccounts(testcpplite::TestResult &result) {
  const auto stream{std::make_shared<std::stringstream>()};
  WritesAccountToStream accountSerialization{*stream};
  accountSerialization.save(
      "jeff",
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}},
        true},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
       {{1256_cents, "walmart", Date{2021, Month::June, 15}}, true},
       {{324_cents, "hyvee", Date{2021, Month::February, 8}}}});
  assertEqual(result, R"(
jeff
credits
^50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
^12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021
)",
              '\n' + stream->str() + '\n');
}

void fromSession(testcpplite::TestResult &result) {
  const auto stream{std::make_shared<std::stringstream>()};
  IoStreamFactoryStub streamFactory{stream};
  StreamAccountSerializationFactoryStub accountSerializationFactory;
  WritesSessionToStream sessionSerialization{streamFactory,
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
                        const VerifiableTransactions &expected,
                        const VerifiableTransactions &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (VerifiableTransactions::size_type i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
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
)")};
  ReadsAccountFromStream accountDeserialization{*input};
  AccountDeserializationObserverStub observer;
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

void toTransactionRecord(testcpplite::TestResult &result) {
  const auto input{std::make_shared<std::stringstream>("^3.24 hyvee 2/8/2020")};
  ReadsTransactionRecordFromStream transactionRecordDeserialization{*input};
  TransactionRecordDeserializationObserverStub observer;
  transactionRecordDeserialization.load(observer);
  assertEqual(result,
              {{324_cents, "hyvee", Date{2020, Month::February, 8}}, true},
              observer.transaction());
}

void toSession(testcpplite::TestResult &result) {
  const auto input{
      std::make_shared<std::stringstream>("jeff\nsteve\nsue\nallen")};
  IoStreamFactoryStub streamFactory{input};
  StreamAccountDeserializationFactoryStub accountDeserializationFactory;
  ReadsSessionFromStream deserialization{streamFactory,
                                         accountDeserializationFactory};
  SessionDeserializationObserverStub observer;
  deserialization.load(observer);
  assertEqual(result, "jeff", observer.primaryAccountName());
  assertEqual(result, "steve", observer.secondaryAccountNames().at(0));
  assertEqual(result, "sue", observer.secondaryAccountNames().at(1));
  assertEqual(result, "allen", observer.secondaryAccountNames().at(2));
}
} // namespace sbash64::budget::stream
