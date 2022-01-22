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
  virtual void updateBalance(std::string_view) = 0;
  virtual void updateAllocation(std::string_view) = 0;
  virtual void addTransaction(std::string_view amount, std::string_view date,
                              std::string_view description,
                              gsl::index index) = 0;
};

class AccountPresenter;

class TransactionPresenter : public ObservableTransaction::Observer {
public:
  TransactionPresenter(AccountView &, AccountPresenter &);
  void notifyThatIsVerified() override;
  void notifyThatIsArchived() override;
  void notifyThatIs(const Transaction &t) override;
  void notifyThatWillBeRemoved() override;

private:
  Transaction transaction;
  AccountView &view;
  AccountPresenter &parent;
};

class AccountPresenter : public Account::Observer {
public:
  explicit AccountPresenter(AccountView &view);
  void notifyThatBalanceHasChanged(USD) override;
  void notifyThatAllocationHasChanged(USD) override;
  void notifyThatHasBeenAdded(ObservableTransaction &t) override;
  void notifyThatWillBeRemoved() override;
  void notifyThatIs(const TransactionPresenter *, const Transaction &);
  auto index(const Transaction &) -> gsl::index;

private:
  AccountView &view;
  std::vector<std::unique_ptr<TransactionPresenter>> transactionPresenters;
  std::vector<Transaction> transactions;
};
} // namespace sbash64::budget

#endif
