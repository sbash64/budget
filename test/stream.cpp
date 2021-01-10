#include "stream.hpp"
#include "usd.hpp"
#include <sbash64/budget/stream.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
#include <utility>

namespace sbash64::budget::file {
namespace {
class IoStreamFactoryStub : public IoStreamFactory {
public:
  explicit IoStreamFactoryStub(std::shared_ptr<std::iostream> stream)
      : stream{std::move(stream)} {}

  auto make() -> std::shared_ptr<std::iostream> override { return stream; }

private:
  std::shared_ptr<std::iostream> stream;
};

class SaveAccountStub : public Account {
public:
  SaveAccountStub(std::string name, std::vector<Transaction> credits,
                  std::vector<Transaction> debits)
      : name{std::move(name)}, credits{std::move(credits)}, debits{std::move(
                                                                debits)} {}

  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void show(View &) override {}
  void load(InputPersistentMemory &) override {}

  void save(OutputPersistentMemory &p) override {
    p.saveAccount(name, credits, debits);
  }

private:
  std::string name;
  std::vector<Transaction> credits;
  std::vector<Transaction> debits;
};

class LoadAccountStub : public Account {
public:
  LoadAccountStub(std::vector<Transaction> &credits,
                  std::vector<Transaction> &debits)
      : credits{credits}, debits{debits} {}

  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void show(View &) override {}
  void save(OutputPersistentMemory &) override {}
  void load(InputPersistentMemory &p) override {
    p.loadAccount(credits, debits);
  }

private:
  std::vector<Transaction> &credits;
  std::vector<Transaction> &debits;
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
  OutputStream file{factory};
  SaveAccountStub jeff{
      "jeff",
      {Transaction{5000_cents, "transfer from master",
                   Date{2021, Month::January, 10}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::March, 12}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::February, 8}}},
      {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
       Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
       Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}}};
  SaveAccountStub steve{
      "steve",
      {Transaction{5000_cents, "transfer from master",
                   Date{2021, Month::January, 10}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::March, 12}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::February, 8}}},
      {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
       Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
       Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}}};
  SaveAccountStub sue{
      "sue",
      {Transaction{5000_cents, "transfer from master",
                   Date{2021, Month::January, 10}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::March, 12}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::February, 8}}},
      {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
       Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
       Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}}};
  SaveAccountStub allen{
      "allen",
      {Transaction{5000_cents, "transfer from master",
                   Date{2021, Month::January, 10}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::March, 12}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::February, 8}}},
      {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
       Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
       Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}}};
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
                        const std::vector<Transaction> &expected,
                        const std::vector<Transaction> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<Transaction>::size_type i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

void loadsAccounts(testcpplite::TestResult &result) {
  auto input{std::make_shared<std::stringstream>(
      R"(jeff
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
)")};
  IoStreamFactoryStub factory{input};
  InputStream file{factory};
  std::vector<Transaction> creditsJeff;
  std::vector<Transaction> debitsJeff;
  std::vector<Transaction> creditsSteve;
  std::vector<Transaction> debitsSteve;
  std::vector<Transaction> creditsSue;
  std::vector<Transaction> debitsSue;
  std::vector<Transaction> creditsAllen;
  std::vector<Transaction> debitsAllen;
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
  assertEqual(result,
              {Transaction{5000_cents, "transfer from master",
                           Date{2021, Month::January, 10}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::March, 12}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::February, 8}}},
              creditsJeff);
  assertEqual(result,
              {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
               Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
               Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}},
              debitsJeff);
  assertEqual(result,
              {Transaction{5000_cents, "transfer from master",
                           Date{2021, Month::January, 10}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::March, 12}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::February, 8}}},
              creditsSteve);
  assertEqual(result,
              {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
               Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
               Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}},
              debitsSteve);
  assertEqual(result,
              {Transaction{5000_cents, "transfer from master",
                           Date{2021, Month::January, 10}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::March, 12}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::February, 8}}},
              creditsSue);
  assertEqual(result,
              {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
               Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
               Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}},
              debitsSue);
  assertEqual(result,
              {Transaction{5000_cents, "transfer from master",
                           Date{2021, Month::January, 10}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::March, 12}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::February, 8}}},
              creditsAllen);
  assertEqual(result,
              {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
               Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
               Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}},
              debitsAllen);
}
} // namespace sbash64::budget::file
