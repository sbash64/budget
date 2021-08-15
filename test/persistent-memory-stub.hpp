#ifndef SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_
#define SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_

#include <sbash64/budget/domain.hpp>

#include <functional>
#include <map>
#include <string_view>
#include <utility>

namespace sbash64::budget {
class PersistentMemoryStub : public BudgetDeserialization,
                             public BudgetSerialization {
public:
  auto primaryAccount() -> const SerializableAccount * {
    return primaryAccount_;
  }

  void save(SerializableAccount &primary,
            const std::vector<SerializableAccount *> &secondaries) override {
    primaryAccount_ = &primary;
    secondaryAccounts_ = secondaries;
  }

  auto secondaryAccounts() -> std::vector<SerializableAccount *> {
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

  auto accountFactory() -> const ExpenseAccount::Factory * {
    return accountFactory_;
  }

private:
  std::vector<SerializableAccount *> secondaryAccounts_;
  const SerializableAccount *primaryAccount_{};
  const std::shared_ptr<Account> *primaryAccountToLoadInto_{};
  const std::map<std::string, std::shared_ptr<Account>, std::less<>>
      *secondaryAccountsToLoadInto_{};
  const ExpenseAccount::Factory *accountFactory_{};
  const Observer *observer_{};
};

class PersistentAccountStub : public AccountDeserialization,
                              public AccountSerialization {
public:
  auto accountName() -> std::string { return accountName_; }

  void save(std::string_view name, USD funds,
            const std::vector<SerializableTransaction *> &credits,
            const std::vector<SerializableTransaction *> &debits) override {
    accountName_ = name;
    funds_ = funds;
    credits_ = credits;
    debits_ = debits;
  }

  auto credits() -> std::vector<SerializableTransaction *> { return credits_; }

  auto debits() -> std::vector<SerializableTransaction *> { return debits_; }

  auto funds() -> USD { return funds_; }

  void setCreditsToLoad(std::vector<VerifiableTransaction> t) {
    creditsToLoad = std::move(t);
  }

  void setDebitsToLoad(std::vector<VerifiableTransaction> t) {
    debitsToLoad = std::move(t);
  }

  void load(Observer &observer) override { observer_ = &observer; }

  auto observer() -> const Observer * { return observer_; }

private:
  std::vector<VerifiableTransaction> creditsToLoad;
  std::vector<VerifiableTransaction> debitsToLoad;
  std::string accountName_;
  std::vector<SerializableTransaction *> credits_;
  std::vector<SerializableTransaction *> debits_;
  USD funds_{};
  const Observer *observer_{};
};
} // namespace sbash64::budget

#endif
