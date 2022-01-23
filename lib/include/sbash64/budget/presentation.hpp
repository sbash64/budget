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
  TransactionPresenter(AccountView &, AccountPresenter &);
  void notifyThatIsVerified() override;
  void notifyThatIsArchived() override;
  void notifyThatIs(const Transaction &t) override;
  void notifyThatWillBeRemoved() override;
  [[nodiscard]] auto get() const -> const Transaction & { return transaction; }

private:
  Transaction transaction;
  AccountView &view;
  AccountPresenter &parent;
};

class AccountPresenter : public Account::Observer {
public:
  explicit AccountPresenter(AccountView &);
  void notifyThatBalanceHasChanged(USD) override;
  void notifyThatAllocationHasChanged(USD) override;
  void notifyThatHasBeenAdded(ObservableTransaction &) override;
  void notifyThatWillBeRemoved() override;
  void ready(const TransactionPresenter *);
  auto index(const TransactionPresenter *) -> gsl::index;
  void remove(const TransactionPresenter *);

private:
  AccountView &view;
  std::vector<std::unique_ptr<TransactionPresenter>> childrenMemory;
  std::vector<const TransactionPresenter *> orderedChildren;
};

class BudgetView {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(BudgetView);
  virtual void addNewAccountTable(gsl::index) = 0;
};

static auto placement(const std::vector<std::string> &orderedNames,
                      std::string_view name) -> gsl::index {
  return distance(orderedNames.begin(),
                  upper_bound(orderedNames.begin(), orderedNames.end(), name));
}

class BudgetPresenter : public Budget::Observer {
public:
  explicit BudgetPresenter(BudgetView &view) : view{view} {}
  void notifyThatExpenseAccountHasBeenCreated(Account &,
                                              std::string_view name) override {
    view.addNewAccountTable(budget::placement(orderedNames, name) + 1);
    orderedNames.insert(
        upper_bound(orderedNames.begin(), orderedNames.end(), name),
        std::string{name});
  }
  void notifyThatNetIncomeHasChanged(USD) override {}

private:
  BudgetView &view;
  std::vector<std::string> orderedNames;
};
} // namespace sbash64::budget

#endif
