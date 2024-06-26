#ifndef SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_
#define SBASH64_BUDGET_TEST_PERSISTENT_MEMORY_STUB_HPP_

#include <sbash64/budget/domain.hpp>

#include <functional>
#include <map>

namespace sbash64::budget {
class PersistentMemoryStub : public BudgetDeserialization,
                             public BudgetSerialization {
public:
  void save(SerializableAccount *incomeAccountWithFunds,
            const std::vector<SerializableAccountWithName>
                &expenseAccountsWithFunds) override {
    incomeAccountWithFunds_ = incomeAccountWithFunds;
    expenseAccountsWithFunds_ = expenseAccountsWithFunds;
  }

  auto incomeAccountWithFunds() -> SerializableAccount * {
    return incomeAccountWithFunds_;
  }

  auto expenseAccountsWithNames() -> std::vector<SerializableAccountWithName> {
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
  std::vector<SerializableAccountWithName> expenseAccountsWithFunds_;
  SerializableAccount *incomeAccountWithFunds_;
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

  void save(const std::vector<SerializableTransaction *> &transactions,
            USD usd) override {
    transactions_ = transactions;
    allocation_ = usd;
  }

  auto transactions() -> std::vector<SerializableTransaction *> {
    return transactions_;
  }

  auto allocation() -> USD { return allocation_; }

  void load(Observer &observer) override { observer_ = &observer; }

  auto observer() -> const Observer * { return observer_; }

private:
  std::string accountName_;
  std::vector<SerializableTransaction *> transactions_;
  USD allocation_{};
  const Observer *observer_{};
};
} // namespace sbash64::budget

#endif
