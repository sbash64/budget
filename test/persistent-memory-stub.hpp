#ifndef SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_
#define SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_

#include <sbash64/budget/budget.hpp>

namespace sbash64::budget {
class PersistentMemoryStub : public PersistentMemory {
public:
  auto primaryAccount() -> const Account * { return primaryAccount_; }

  void save(Account &primary,
            const std::vector<Account *> &secondaries) override {
    primaryAccount_ = &primary;
    secondaryAccounts_ = secondaries;
  }

  auto secondaryAccounts() -> std::vector<Account *> {
    return secondaryAccounts_;
  }

private:
  std::vector<Account *> secondaryAccounts_;
  const Account *primaryAccount_{};
};
} // namespace sbash64::budget

#endif
