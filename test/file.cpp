#include "file.hpp"
#include <sbash64/budget/file.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

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
  File file;
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
} // namespace sbash64::budget::file
