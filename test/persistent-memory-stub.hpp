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

  auto accountName() -> std::string { return accountName_; }

  void saveAccount(std::string_view name,
                   const std::vector<Transaction> &credits,
                   const std::vector<Transaction> &debits) {
    accountName_ = name;
    credits_ = credits;
    debits_ = debits;
  }

  auto credits() -> std::vector<Transaction> { return credits_; }

  auto debits() -> std::vector<Transaction> { return debits_; }

private:
  std::string accountName_;
  std::vector<Account *> secondaryAccounts_;
  std::vector<Transaction> credits_;
  std::vector<Transaction> debits_;
  const Account *primaryAccount_{};
};
} // namespace sbash64::budget

#endif
