#ifndef SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_
#define SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_

#include <functional>
#include <map>
#include <sbash64/budget/budget.hpp>
#include <string_view>
#include <utility>

namespace sbash64::budget {
class PersistentMemoryStub : public SessionDeserialization,
                             public SessionSerialization {
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

  auto primaryAccountToLoadInto() -> const std::shared_ptr<Account> * {
    return primaryAccountToLoadInto_;
  }

  void load(Observer &observer) override { observer_ = &observer; }

  auto observer() -> const Observer * { return observer_; }

  auto secondaryAccountsToLoadInto()
      -> const std::map<std::string, std::shared_ptr<Account>, std::less<>> * {
    return secondaryAccountsToLoadInto_;
  }

  auto accountFactory() -> const Account::Factory * { return accountFactory_; }

private:
  std::vector<Account *> secondaryAccounts_;
  const Account *primaryAccount_{};
  const std::shared_ptr<Account> *primaryAccountToLoadInto_{};
  const std::map<std::string, std::shared_ptr<Account>, std::less<>>
      *secondaryAccountsToLoadInto_{};
  const Account::Factory *accountFactory_{};
  const Observer *observer_{};
};

class PersistentAccountStub : public AccountDeserialization,
                              public AccountSerialization {
public:
  auto accountName() -> std::string { return accountName_; }

  void save(std::string_view name, const VerifiableTransactions &credits,
            const VerifiableTransactions &debits) override {
    accountName_ = name;
    credits_ = credits;
    debits_ = debits;
  }

  auto credits() -> VerifiableTransactions { return credits_; }

  auto debits() -> VerifiableTransactions { return debits_; }

  void setCreditsToLoad(VerifiableTransactions t) {
    creditsToLoad = std::move(t);
  }

  void setDebitsToLoad(VerifiableTransactions t) {
    debitsToLoad = std::move(t);
  }

  void load(Observer &observer) override { observer_ = &observer; }

  auto observer() -> const Observer * { return observer_; }

private:
  VerifiableTransactions creditsToLoad;
  VerifiableTransactions debitsToLoad;
  std::string accountName_;
  VerifiableTransactions credits_;
  VerifiableTransactions debits_;
  const Observer *observer_{};
};
} // namespace sbash64::budget

#endif
