#ifndef SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_
#define SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_

#include <functional>
#include <map>
#include <sbash64/budget/budget.hpp>
#include <string_view>
#include <utility>

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
                   const std::vector<Transaction> &debits) override {
    accountName_ = name;
    credits_ = credits;
    debits_ = debits;
  }

  auto credits() -> std::vector<Transaction> { return credits_; }

  auto debits() -> std::vector<Transaction> { return debits_; }

  void setCreditsToLoad(std::string_view name, std::vector<Transaction> t) {
    creditsToLoad[std::string{name}] = std::move(t);
  }

  void setDebitsToLoad(std::string_view name, std::vector<Transaction> t) {
    debitsToLoad[std::string{name}] = std::move(t);
  }

  void loadAccount(std::string_view name, std::vector<Transaction> &credits,
                   std::vector<Transaction> &debits) override {
    credits = creditsToLoad.at(std::string{name});
    debits = debitsToLoad.at(std::string{name});
  }

private:
  std::map<std::string, std::vector<Transaction>> creditsToLoad;
  std::map<std::string, std::vector<Transaction>> debitsToLoad;
  std::string accountName_;
  std::vector<Account *> secondaryAccounts_;
  std::vector<Transaction> credits_;
  std::vector<Transaction> debits_;
  const Account *primaryAccount_{};
};
} // namespace sbash64::budget

#endif
