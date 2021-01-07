#include "file.hpp"
#include "usd.hpp"
#include <sbash64/budget/file.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>

namespace sbash64::budget::file {
namespace {
class AccountStub : public Account {
public:
  void credit(const Transaction &) override {}
  void debit(const Transaction &) override {}
  void show(View &) override {}

  void save(PersistentMemory &p) override { persistentMemory_ = &p; }

  auto persistentMemory() -> const PersistentMemory * {
    return persistentMemory_;
  }

private:
  const PersistentMemory *persistentMemory_{};
};
} // namespace

static void assertSaved(testcpplite::TestResult &result, AccountStub &account,
                        PersistentMemory &persistentMemory) {
  assertEqual(result, &persistentMemory, account.persistentMemory());
}

void savesAccounts(testcpplite::TestResult &result) {
  std::stringstream stream;
  std::stringstream input;
  File file{input, stream};
  AccountStub primary;
  AccountStub jim;
  AccountStub allison;
  AccountStub ned;
  file.save(primary, {&jim, &allison, &ned});
  assertSaved(result, primary, file);
  assertSaved(result, jim, file);
  assertSaved(result, allison, file);
  assertSaved(result, ned, file);
}

void savesAccount(testcpplite::TestResult &result) {
  std::stringstream stream;
  std::stringstream input;
  File file{input, stream};
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
3.24 hyvee 2/8/2021
)",
              '\n' + stream.str() + '\n');
}

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<Transaction> &expected,
                        const std::vector<Transaction> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (int i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

void loadsAccount(testcpplite::TestResult &result) {
  std::stringstream stream{R"(
Groceries
credits
50 transfer from master 1/10/2021
25 transfer from master 3/12/2021
25 transfer from master 2/8/2021
debits
27.34 hyvee 1/12/2021
12.56 walmart 6/15/2021
3.24 hyvee 2/8/2021
)"};
  std::stringstream output;
  File file{stream, output};
  std::vector<Transaction> credits;
  std::vector<Transaction> debits;
  file.loadAccount("Groceries", credits, debits);
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
}
} // namespace sbash64::budget::file
