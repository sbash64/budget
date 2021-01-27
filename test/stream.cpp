#include "stream.hpp"
#include "usd.hpp"
#include <sbash64/budget/serialization.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
#include <utility>

namespace sbash64::budget::file {
namespace {
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

  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void removeCredit(const Transaction &) override {}
  void removeDebit(const Transaction &) override {}
  void show(View &) override {}
  void load(SessionDeserialization &) override {}
  void rename(std::string_view) override {}
  auto findUnverifiedDebits(USD) -> Transactions override { return {}; }
  void verifyDebit(const Transaction &) override {}
  void verifyCredit(const Transaction &) override {}

  void save(SessionSerialization &p) override {
    p.saveAccount(name, credits, debits);
  }

private:
  std::string name;
  VerifiableTransactions credits;
  VerifiableTransactions debits;
};

class LoadAccountStub : public Account {
public:
  LoadAccountStub(VerifiableTransactions &credits,
                  VerifiableTransactions &debits)
      : credits{credits}, debits{debits} {}

  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void removeCredit(const Transaction &) override {}
  void removeDebit(const Transaction &) override {}
  void show(View &) override {}
  void save(SessionSerialization &) override {}
  void rename(std::string_view) override {}
  auto findUnverifiedDebits(USD) -> Transactions override { return {}; }
  void verifyDebit(const Transaction &) override {}
  void verifyCredit(const Transaction &) override {}

  void load(SessionDeserialization &p) override {
    p.loadAccount(credits, debits);
  }

private:
  VerifiableTransactions &credits;
  VerifiableTransactions &debits;
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
  IoStreamFactoryStub factory{stream};
  WritesSessionToStream file{factory};
  SaveAccountStub jeff{
      "jeff",
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
       {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
       {{324_cents, "hyvee", Date{2021, Month::February, 8}}}}};
  SaveAccountStub steve{
      "steve",
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
       {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
       {{324_cents, "hyvee", Date{2021, Month::February, 8}}}}};
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
       {{324_cents, "hyvee", Date{2021, Month::February, 8}}}}};
  file.save(jeff, {&steve, &sue, &allen});
  assertEqual(result, R"(
jeff
credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021

steve
credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021

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
3.24 hyvee 2/8/2021
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
27.34 hyvee 1/12/2021
9.87 walmart 6/15/2021
3.24 hyvee 2/8/2020

steve
credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
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
3.24 hyvee 2/8/2021
)")};
  IoStreamFactoryStub factory{input};
  ReadsSessionFromStream file{factory};
  VerifiableTransactions creditsJeff;
  VerifiableTransactions debitsJeff;
  VerifiableTransactions creditsSteve;
  VerifiableTransactions debitsSteve;
  VerifiableTransactions creditsSue;
  VerifiableTransactions debitsSue;
  VerifiableTransactions creditsAllen;
  VerifiableTransactions debitsAllen;
  const auto jeff{std::make_shared<LoadAccountStub>(creditsJeff, debitsJeff)};
  const auto steve{
      std::make_shared<LoadAccountStub>(creditsSteve, debitsSteve)};
  const auto sue{std::make_shared<LoadAccountStub>(creditsSue, debitsSue)};
  const auto allen{
      std::make_shared<LoadAccountStub>(creditsAllen, debitsAllen)};
  AccountFactoryStub accountFactory;
  accountFactory.add(jeff, "jeff");
  accountFactory.add(steve, "steve");
  accountFactory.add(sue, "sue");
  accountFactory.add(allen, "allen");
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
  std::shared_ptr<Account> primary;
  file.load(accountFactory, primary, accounts);
  assertEqual(result, jeff.get(), primary.get());
  assertEqual(result, steve.get(), accounts.at("steve").get());
  assertEqual(result, sue.get(), accounts.at("sue").get());
  assertEqual(result, allen.get(), accounts.at("allen").get());
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::April, 12}}},
       {{1380_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      creditsJeff);
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
               {{987_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2020, Month::February, 8}}}},
              debitsJeff);
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      creditsSteve);
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
               {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2021, Month::February, 8}}}},
              debitsSteve);
  assertEqual(
      result,
      {{{7500_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{2500_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      creditsSue);
  assertEqual(result,
              {{{2734_cents, "bakers", Date{2021, Month::January, 12}}},
               {{1256_cents, "walmart", Date{2021, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2021, Month::February, 8}}}},
              debitsSue);
  assertEqual(
      result,
      {{{5000_cents, "transfer from master", Date{2021, Month::January, 10}}},
       {{3200_cents, "transfer from master", Date{2021, Month::March, 12}}},
       {{2500_cents, "transfer from master", Date{2021, Month::February, 8}}}},
      creditsAllen);
  assertEqual(result,
              {{{2734_cents, "hyvee", Date{2021, Month::January, 12}}},
               {{1256_cents, "walmart", Date{1984, Month::June, 15}}},
               {{324_cents, "hyvee", Date{2021, Month::February, 8}}}},
              debitsAllen);
}
} // namespace sbash64::budget::file
