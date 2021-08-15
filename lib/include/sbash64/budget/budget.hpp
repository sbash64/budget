#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include "domain.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace sbash64::budget {
class BudgetInMemory : public Budget {
public:
  explicit BudgetInMemory(IncomeAccount &, ExpenseAccount::Factory &);
  void attach(Observer *) override;
  void credit(const Transaction &) override;
  void debit(std::string_view accountName, const Transaction &) override;
  void removeCredit(const Transaction &) override;
  void removeDebit(std::string_view accountName, const Transaction &) override;
  void verifyCredit(const Transaction &) override;
  void verifyDebit(std::string_view accountName, const Transaction &) override;
  void transferTo(std::string_view accountName, USD amount) override;
  void allocate(std::string_view accountName, USD) override;
  void createAccount(std::string_view name) override;
  void closeAccount(std::string_view name) override;
  void removeAccount(std::string_view name) override;
  void renameAccount(std::string_view from, std::string_view to) override;
  void reduce() override;
  void restore() override;
  void save(BudgetSerialization &) override;
  void load(BudgetDeserialization &) override;
  void notifyThatPrimaryAccountIsReady(AccountDeserialization &,
                                       std::string_view name) override;
  void notifyThatSecondaryAccountIsReady(AccountDeserialization &,
                                         std::string_view name) override;

private:
  ExpenseAccount::Factory &accountFactory;
  Observer *observer{};
  IncomeAccount &incomeAccount;
  std::map<std::string, std::shared_ptr<Account>, std::less<>> expenseAccounts;
};
} // namespace sbash64::budget

#endif
