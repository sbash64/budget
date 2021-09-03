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
  void save(SerializableAccountWithFunds incomeAccountWithFunds,
            const std::vector<SerializableAccountWithFundsAndName>
                &expenseAccountsWithFunds) override {
    incomeAccountWithFunds_ = incomeAccountWithFunds;
    expenseAccountsWithFunds_ = expenseAccountsWithFunds;
  }

  auto incomeAccountWithFunds() -> SerializableAccountWithFunds {
    return incomeAccountWithFunds_;
  }

  auto expenseAccountsWithFunds()
      -> std::vector<SerializableAccountWithFundsAndName> {
    return expenseAccountsWithFunds_;
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
  std::vector<SerializableAccountWithFundsAndName> expenseAccountsWithFunds_;
  SerializableAccountWithFunds incomeAccountWithFunds_;
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

  void
  save(const std::vector<SerializableTransaction *> &transactions) override {
    transactions_ = transactions;
  }

  auto transactions() -> std::vector<SerializableTransaction *> {
    return transactions_;
  }

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
  std::vector<SerializableTransaction *> transactions_;
  USD funds_{};
  const Observer *observer_{};
};
} // namespace sbash64::budget

#endif
