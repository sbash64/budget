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

  auto accountName() -> std::string { return accountName_; }

  void saveAccount(std::string_view name, const VerifiableTransactions &credits,
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

  void loadAccount(VerifiableTransactions &credits,
                   VerifiableTransactions &debits) override {
    credits = creditsToLoad;
    debits = debitsToLoad;
  }

  auto primaryAccountToLoadInto() -> const std::shared_ptr<Account> * {
    return primaryAccountToLoadInto_;
  }

  void load(Account::Factory &factory, std::shared_ptr<Account> &primary,
            std::map<std::string, std::shared_ptr<Account>, std::less<>>
                &secondaries) override {
    accountFactory_ = &factory;
    primaryAccountToLoadInto_ = &primary;
    secondaryAccountsToLoadInto_ = &secondaries;
  }

  auto secondaryAccountsToLoadInto()
      -> const std::map<std::string, std::shared_ptr<Account>, std::less<>> * {
    return secondaryAccountsToLoadInto_;
  }

  auto accountFactory() -> const Account::Factory * { return accountFactory_; }

private:
  VerifiableTransactions creditsToLoad;
  VerifiableTransactions debitsToLoad;
  std::string accountName_;
  std::vector<Account *> secondaryAccounts_;
  VerifiableTransactions credits_;
  VerifiableTransactions debits_;
  const Account *primaryAccount_{};
  const std::shared_ptr<Account> *primaryAccountToLoadInto_{};
  const std::map<std::string, std::shared_ptr<Account>, std::less<>>
      *secondaryAccountsToLoadInto_{};
  const Account::Factory *accountFactory_{};
};
} // namespace sbash64::budget

#endif
