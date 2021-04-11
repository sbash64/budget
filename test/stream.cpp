#include "stream.hpp"
#include "usd.hpp"
#include <sbash64/budget/serialization.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
#include <utility>

namespace sbash64::budget::file {
namespace {
class AccountSerializationStub : public AccountSerialization {
public:
  void save(std::string_view name, const VerifiableTransactions &credits,
            const VerifiableTransactions &debits) override {}
};

class StreamAccountSerializationFactoryStub
    : public StreamAccountSerializationFactory {
public:
  explicit StreamAccountSerializationFactoryStub(
      std::shared_ptr<AccountSerialization> accountSerialization)
      : accountSerialization{std::move(accountSerialization)} {}

  auto make(std::ostream &) -> std::shared_ptr<AccountSerialization> override {
    return accountSerialization;
  }

private:
  std::shared_ptr<AccountSerialization> accountSerialization;
};

class SessionDeserializationObserverStub
    : public SessionDeserialization::Observer {
public:
  explicit SessionDeserializationObserverStub(Account::Factory &factory)
      : factory{factory} {}

  void notifyThatPrimaryAccountIsReady(AccountDeserialization &deserialization,
                                       std::string_view name) override {
    factory.make(name)->load(deserialization);
  }

  void
  notifyThatSecondaryAccountIsReady(AccountDeserialization &deserialization,
                                    std::string_view name) override {
    factory.make(name)->load(deserialization);
  }

private:
  Account::Factory &factory;
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

class SaveAccountStub : public Account {
public:
  SaveAccountStub(std::string name, VerifiableTransactions credits,
                  VerifiableTransactions debits)
      : name{std::move(name)}, credits{std::move(credits)}, debits{std::move(
                                                                debits)} {}
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

  void save(AccountSerialization &p) override { p.save(name, credits, debits); }

private:
  std::string name;
  VerifiableTransactions credits;
  VerifiableTransactions debits;
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

  void save(AccountSerialization &p) override { stream << name; }

private:
  std::string name;
  std::ostream &stream;
};

class LoadAccountStub : public Account {
public:
  void attach(Observer *) override {}
  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void removeCredit(const Transaction &) override {}
  void removeDebit(const Transaction &) override {}
  void show(View &) override {}
  void save(AccountSerialization &) override {}
  void rename(std::string_view) override {}
  auto findUnverifiedDebits(USD) -> Transactions override { return {}; }
  auto findUnverifiedCredits(USD) -> Transactions override { return {}; }
  void verifyDebit(const Transaction &) override {}
  void verifyCredit(const Transaction &) override {}
  void
  notifyThatDebitHasBeenDeserialized(const VerifiableTransaction &t) override {
    debits_.push_back(t);
  }

  void
  notifyThatCreditHasBeenDeserialized(const VerifiableTransaction &t) override {
    credits_.push_back(t);
  }

  void load(AccountDeserialization &p) override { p.load(*this); }

  auto credits() -> VerifiableTransactions { return credits_; }

  auto debits() -> VerifiableTransactions { return debits_; }

private:
  VerifiableTransactions credits_;
  VerifiableTransactions debits_;
};

class AccountFactoryStub : public Account::Factory {
public:
  void add(std::shared_ptr<Account> account, std::string_view name) {
    accounts[std::string{name}] = std::move(account);
  }

  auto make(std::string_view s) -> std::shared_ptr<Account> override {
    return accounts.at(std::string{s});
  }

private:
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
};
} // namespace

void savesAccounts(testcpplite::TestResult &result) {
  auto stream{std::make_shared<std::stringstream>()};
  IoStreamFactoryStub streamFactory{stream};
  WritesAccountToStream::Factory accountSerializationFactory;
  WritesSessionToStream file{streamFactory, accountSerializationFactory};
  SaveAccountStub jeff{
      "jeff",
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}},
        true},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
       {{1256_cents, "walmart", Date{2021, Month::June, 15}}, true},
       {{324_cents, "hyvee", Date{2021, Month::February, 8}}}}};
  SaveAccountStub steve{
      "steve",
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
       {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
       {{324_cents, "hyvee", Date{2021, Month::February, 8}}, true}}};
  SaveAccountStub sue{
      "sue",
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
       {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
       {{324_cents, "hyvee", Date{2021, Month::February, 8}}}}};
  SaveAccountStub allen{
      "allen",
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
       {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
       {{304_cents, "hyvee", Date{2021, Month::February, 8}}}}};
  file.save(jeff, {&steve, &sue, &allen});
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

steve
credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
^3.24 hyvee 2/8/2021

sue
credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021

allen
credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
3.04 hyvee 2/8/2021
)",
              '\n' + stream->str());
}

void savesSession(testcpplite::TestResult &result) {
  const auto stream{std::make_shared<std::stringstream>()};
  IoStreamFactoryStub streamFactory{stream};
  const auto accountSerialization{std::make_shared<AccountSerializationStub>()};
  StreamAccountSerializationFactoryStub accountSerializationFactory{
      accountSerialization};
  WritesSessionToStream file{streamFactory, accountSerializationFactory};
  SavesNameAccountStub jeff{*stream, "jeff"};
  SavesNameAccountStub steve{*stream, "steve"};
  SavesNameAccountStub sue{*stream, "sue"};
  SavesNameAccountStub allen{*stream, "allen"};
  file.save(jeff, {&steve, &sue, &allen});
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

void loadsAccounts(testcpplite::TestResult &result) {
  const auto input{std::make_shared<std::stringstream>(
      R"(jeff
credits
50 transfer from master 1/10/2021
25 transfer from master 4/12/2021
13.80 transfer from master 2/8/2021
debits
^27.34 hyvee 1/12/2021
9.87 walmart 6/15/2021
3.24 hyvee 2/8/2020

steve
credits
^50 transfer from master 1/10/2021
^25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021

sue
credits
75 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 bakers 1/12/2021
12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021

allen
credits
50 transfer from master 1/10/2021
32 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/1984
3.04 hyvee 2/8/2021
)")};
  IoStreamFactoryStub factory{input};
  ReadsAccountFromStream::Factory accountDeserializationFactory;
  ReadsSessionFromStream file{factory, accountDeserializationFactory};
  const auto jeff{std::make_shared<LoadAccountStub>()};
  const auto steve{std::make_shared<LoadAccountStub>()};
  const auto sue{std::make_shared<LoadAccountStub>()};
  const auto allen{std::make_shared<LoadAccountStub>()};
  AccountFactoryStub accountFactory;
  accountFactory.add(jeff, "jeff");
  accountFactory.add(steve, "steve");
  accountFactory.add(sue, "sue");
  accountFactory.add(allen, "allen");
  SessionDeserializationObserverStub observer{accountFactory};
  file.load(observer);
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::April, 12}}},
       {{1380_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      jeff->credits());
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}, true},
               {{987_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2020, Month::February, 8}}}},
              jeff->debits());
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}},
        true},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}},
        true},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      steve->credits());
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
               {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2021, Month::February, 8}}}},
              steve->debits());
  assertEqual(
      result,
      {{{7500_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      sue->credits());
  assertEqual(result,
              {{{2734_cents, "bakers", Date{2021, Month::January, 12}}},
               {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2021, Month::February, 8}}}},
              sue->debits());
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{3200_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      allen->credits());
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
               {{1256_cents, "walmart", Date{1984, Month::June, 15}}},
               {{304_cents, "hyvee", Date{2021, Month::February, 8}}}},
              allen->debits());
}
} // namespace sbash64::budget::file
