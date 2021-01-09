#include "stream.hpp"
#include "usd.hpp"
#include <sbash64/budget/stream.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>

namespace sbash64::budget::file {
namespace {
class SaveAccountStub : public Account {
public:
  SaveAccountStub(std::ostream &stream, std::string name)
      : stream{stream}, name{std::move(name)} {}

  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void show(View &) override {}
  void load(InputPersistentMemory &) override {}

  void save(OutputPersistentMemory &p) override {
    persistentMemory_ = &p;
    stream << name;
  }

  auto persistentMemory() -> const OutputPersistentMemory * {
    return persistentMemory_;
  }

private:
  const OutputPersistentMemory *persistentMemory_{};
  std::ostream &stream;
  std::string name;
};

class LoadAccountStub : public Account {
public:
  explicit LoadAccountStub(std::istream &stream) : stream{stream} {}

  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void show(View &) override {}
  void save(OutputPersistentMemory &) override {}
  void load(InputPersistentMemory &p) override {
    persistentMemory_ = &p;
    getline(stream, lineRead_);
  }

  auto persistentMemory() -> const InputPersistentMemory * {
    return persistentMemory_;
  }

  auto lineRead() -> std::string { return lineRead_; }

private:
  const InputPersistentMemory *persistentMemory_{};
  std::istream &stream;
  std::string lineRead_;
};

class AccountFactoryStub : public Account::Factory {
public:
  void add(std::shared_ptr<Account> account, std::string_view name) {
    accounts[std::string{name}] = std::move(account);
  }

  auto make(std::string_view s) -> std::shared_ptr<Account> override {
    return accounts.count(s) == 0 ? nullptr : accounts.at(std::string{s});
  }

private:
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
};
} // namespace

static void assertSaved(testcpplite::TestResult &result,
                        SaveAccountStub &account,
                        OutputPersistentMemory &persistentMemory) {
  assertEqual(result, &persistentMemory, account.persistentMemory());
}

static void assertLoaded(testcpplite::TestResult &result,
                         LoadAccountStub &account,
                         InputPersistentMemory &persistentMemory) {
  assertEqual(result, &persistentMemory, account.persistentMemory());
}

void savesAccounts(testcpplite::TestResult &result) {
  std::stringstream stream;
  OutputStream file{stream};
  SaveAccountStub jeff{stream, "jeff"};
  SaveAccountStub steve{stream, "steve"};
  SaveAccountStub sue{stream, "sue"};
  SaveAccountStub allen{stream, "allen"};
  file.save(jeff, {&steve, &sue, &allen});
  assertSaved(result, jeff, file);
  assertSaved(result, steve, file);
  assertSaved(result, sue, file);
  assertSaved(result, allen, file);
  assertEqual(result, R"(
jeff

steve

sue

allen
)",
              '\n' + stream.str());
}

void savesAccount(testcpplite::TestResult &result) {
  std::stringstream stream;
  OutputStream file{stream};
  file.saveAccount(
      "Groceries",
      {Transaction{5000_cents, "transfer from master",
                   Date{2021, Month::January, 10}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::March, 12}},
       Transaction{2500_cents, "transfer from master",
                   Date{2021, Month::February, 8}}},
      {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
       Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
       Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}});
  assertEqual(result, R"(
Groceries
credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021)",
              '\n' + stream.str());
}

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<Transaction> &expected,
                        const std::vector<Transaction> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<Transaction>::size_type i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

void loadsAccount(testcpplite::TestResult &result) {
  std::stringstream stream{
      R"(credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021

next)"};
  InputStream file{stream};
  std::vector<Transaction> credits;
  std::vector<Transaction> debits;
  file.loadAccount(credits, debits);
  assertEqual(result,
              {Transaction{5000_cents, "transfer from master",
                           Date{2021, Month::January, 10}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::March, 12}},
               Transaction{2500_cents, "transfer from master",
                           Date{2021, Month::February, 8}}},
              credits);
  assertEqual(result,
              {Transaction{2734_cents, "hyvee", Date{2021, Month::January, 12}},
               Transaction{1256_cents, "walmart", Date{2021, Month::June, 15}},
               Transaction{324_cents, "hyvee", Date{2021, Month::February, 8}}},
              debits);
  std::string next;
  getline(stream, next);
  assertEqual(result, "next", next);
}

void loadsAccounts(testcpplite::TestResult &result) {
  std::stringstream input{
      R"(jeff
this is for jeff
steve
this one's for steve
sue
and of course sue
allen
last but not least is allen)"};
  InputStream file{input};
  const auto jeff{std::make_shared<LoadAccountStub>(input)};
  const auto steve{std::make_shared<LoadAccountStub>(input)};
  const auto sue{std::make_shared<LoadAccountStub>(input)};
  const auto allen{std::make_shared<LoadAccountStub>(input)};
  AccountFactoryStub factory;
  factory.add(jeff, "jeff");
  factory.add(steve, "steve");
  factory.add(sue, "sue");
  factory.add(allen, "allen");
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
  std::shared_ptr<Account> primary;
  file.load(factory, primary, accounts);
  assertLoaded(result, *jeff, file);
  assertLoaded(result, *steve, file);
  assertLoaded(result, *sue, file);
  assertLoaded(result, *allen, file);
  assertEqual(result, jeff.get(), primary.get());
  assertEqual(result, steve.get(), accounts.at("steve").get());
  assertEqual(result, sue.get(), accounts.at("sue").get());
  assertEqual(result, allen.get(), accounts.at("allen").get());
  assertEqual(result, "this is for jeff", jeff->lineRead());
  assertEqual(result, "this one's for steve", steve->lineRead());
  assertEqual(result, "and of course sue", sue->lineRead());
  assertEqual(result, "last but not least is allen", allen->lineRead());
}
} // namespace sbash64::budget::file
