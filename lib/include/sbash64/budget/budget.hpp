#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include "domain.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace sbash64::budget {
class BudgetInMemory : public Budget {
public:
  using ExpenseAccountsType =
      std::map<std::string, std::shared_ptr<Account>, std::less<>>;

  BudgetInMemory(Account &incomeAccount, Account::Factory &);
  void attach(Observer &) override;
  void addIncome(const Transaction &) override;
  void addExpense(std::string_view accountName, const Transaction &) override;
  void removeIncome(const Transaction &) override;
  void removeExpense(std::string_view accountName,
                     const Transaction &) override;
  void verifyIncome(const Transaction &) override;
  void verifyExpense(std::string_view accountName,
                     const Transaction &) override;
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
  void notifyThatIncomeAccountIsReady(AccountDeserialization &) override;
  void notifyThatExpenseAccountIsReady(AccountDeserialization &,
                                       std::string_view name) override;

private:
  ExpenseAccountsType expenseAccounts;
  Account &incomeAccount;
  Account::Factory &accountFactory;
  std::vector<std::reference_wrapper<Observer>> observers{};
};
} // namespace sbash64::budget

#endif
