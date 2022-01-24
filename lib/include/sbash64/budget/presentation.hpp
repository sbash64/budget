#ifndef SBASH64_BUDGET_PRESENTATION_HPP_
#define SBASH64_BUDGET_PRESENTATION_HPP_

#include "domain.hpp"

#include <gsl/gsl>

#include <memory>
#include <string_view>
#include <vector>

namespace sbash64::budget {
class AccountView {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountView);
  virtual void putCheckmarkNextToTransaction(gsl::index index) = 0;
  virtual void removeTransactionSelection(gsl::index index) = 0;
  virtual void updateBalance(std::string_view) = 0;
  virtual void updateAllocation(std::string_view) = 0;
  virtual void addTransactionRow(std::string_view amount, std::string_view date,
                                 std::string_view description,
                                 gsl::index index) = 0;
  virtual void deleteTransaction(gsl::index) = 0;
};

class AccountPresenter;

class TransactionPresenter : public ObservableTransaction::Observer {
public:
  TransactionPresenter(ObservableTransaction &, AccountView &,
                       AccountPresenter &);
  void notifyThatIsVerified() override;
  void notifyThatIsArchived() override;
  void notifyThatIs(const Transaction &t) override;
  void notifyThatWillBeRemoved() override;
  [[nodiscard]] auto get() const -> const Transaction & { return transaction; }
  void setIndex(gsl::index i) { index = i; }

private:
  Transaction transaction;
  AccountView &view;
  AccountPresenter &parent;
  gsl::index index{-1};
};

class BudgetPresenter;
class BudgetView;

class AccountPresenter : public Account::Observer {
public:
  AccountPresenter(Account &, AccountView &, std::string_view name = "",
                   BudgetPresenter *parent = nullptr,
                   BudgetView *parentView = nullptr);
  void notifyThatBalanceHasChanged(USD) override;
  void notifyThatAllocationHasChanged(USD) override;
  void notifyThatHasBeenAdded(ObservableTransaction &) override;
  void notifyThatWillBeRemoved() override;
  void ready(const TransactionPresenter *);
  void remove(const TransactionPresenter *);
  [[nodiscard]] auto name() const -> std::string { return name_; }
  void setIndex(gsl::index i) { index = i; }

private:
  AccountView &view;
  std::string name_;
  std::vector<std::unique_ptr<TransactionPresenter>> unorderedChildren;
  std::vector<std::unique_ptr<TransactionPresenter>> orderedChildren;
  BudgetPresenter *parent;
  BudgetView *parentView;
  gsl::index index{-1};
};

class BudgetView {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(BudgetView);
  virtual auto addNewAccountTable(std::string_view name, gsl::index)
      -> AccountView & = 0;
  virtual void deleteAccountTable(gsl::index) = 0;
  virtual void updateNetIncome(std::string_view amount) = 0;
};

class BudgetPresenter : public Budget::Observer {
public:
  explicit BudgetPresenter(BudgetView &view);
  void notifyThatExpenseAccountHasBeenCreated(Account &,
                                              std::string_view name) override;
  void notifyThatNetIncomeHasChanged(USD) override;
  void remove(const AccountPresenter *);

private:
  BudgetView &view;
  std::vector<std::unique_ptr<AccountPresenter>> orderedChildren;
};
} // namespace sbash64::budget

#endif
