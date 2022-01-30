#ifndef SBASH64_BUDGET_PRESENTATION_HPP_
#define SBASH64_BUDGET_PRESENTATION_HPP_

#include "domain.hpp"

#include <gsl/gsl>

#include <memory>
#include <string_view>
#include <vector>

namespace sbash64::budget {
class View {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(View);
  virtual void updateNetIncome(std::string_view amount) = 0;
  virtual void addNewAccountTable(std::string_view name,
                                  gsl::index accountIndex) = 0;
  virtual void deleteAccountTable(gsl::index accountIndex) = 0;
  virtual void updateAccountAllocation(gsl::index accountIndex,
                                       std::string_view) = 0;
  virtual void updateAccountBalance(gsl::index accountIndex,
                                    std::string_view) = 0;
  virtual void addTransactionRow(gsl::index accountIndex,
                                 std::string_view amount, std::string_view date,
                                 std::string_view description,
                                 gsl::index transactionIndex) = 0;
  virtual void deleteTransactionRow(gsl::index accountIndex,
                                    gsl::index transactionIndex) = 0;
  virtual void
  putCheckmarkNextToTransactionRow(gsl::index accountIndex,
                                   gsl::index transactionIndex) = 0;
  virtual void removeTransactionRowSelection(gsl::index accountIndex,
                                             gsl::index transactionIndex) = 0;
};

class AccountPresenter;

class TransactionPresenter : public ObservableTransaction::Observer {
public:
  TransactionPresenter(ObservableTransaction &, View &, AccountPresenter &);
  void notifyThatIsVerified() override;
  void notifyThatIsArchived() override;
  void notifyThatIs(const Transaction &t) override;
  void notifyThatWillBeRemoved() override;
  [[nodiscard]] auto get() const -> const Transaction & { return transaction; }
  void setIndex(gsl::index i) { index = i; }

private:
  Transaction transaction;
  View &view;
  AccountPresenter &parent;
  gsl::index index{-1};
};

class BudgetPresenter;
class BudgetView;

class AccountPresenter : public Account::Observer {
public:
  class Parent {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Parent);
    virtual void remove(const AccountPresenter *) = 0;
  };

  AccountPresenter(Account &, View &, std::string_view name, Parent &);
  void notifyThatBalanceHasChanged(USD) override;
  void notifyThatAllocationHasChanged(USD) override;
  void notifyThatHasBeenAdded(ObservableTransaction &) override;
  void notifyThatWillBeRemoved() override;
  void ready(const TransactionPresenter *);
  void remove(const TransactionPresenter *);
  [[nodiscard]] auto name() const -> std::string { return name_; }
  void setIndex(gsl::index i) { index_ = i; }
  [[nodiscard]] auto index() const -> gsl::index { return index_; }

private:
  View &view;
  std::string name_;
  std::vector<std::unique_ptr<TransactionPresenter>> unorderedChildren;
  std::vector<std::unique_ptr<TransactionPresenter>> orderedChildren;
  Parent &parent;
  gsl::index index_{-1};
};

class BudgetPresenter : public Budget::Observer,
                        public AccountPresenter::Parent {
public:
  BudgetPresenter(View &, Account &incomeAccount);
  void notifyThatExpenseAccountHasBeenCreated(Account &,
                                              std::string_view name) override;
  void notifyThatNetIncomeHasChanged(USD) override;
  void remove(const AccountPresenter *) override;

private:
  View &view;
  AccountPresenter incomeAccountPresenter;
  std::vector<std::unique_ptr<AccountPresenter>> orderedChildren;
};
} // namespace sbash64::budget

#endif
