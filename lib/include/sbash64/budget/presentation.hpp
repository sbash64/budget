#ifndef SBASH64_BUDGET_PRESENTATION_HPP_
#define SBASH64_BUDGET_PRESENTATION_HPP_

#include "domain.hpp"
#include "format.hpp"

#include <memory>
#include <sstream>
#include <string_view>
#include <vector>

namespace sbash64::budget {
class AccountView {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountView);
  virtual void addTransaction(std::string_view amount,
                              std::string_view date) = 0;
};

class TransactionPresenter : public ObservableTransaction::Observer {
public:
  explicit TransactionPresenter(AccountView &view) : view{view} {}
  void notifyThatIsVerified() override {}
  void notifyThatIsArchived() override {}
  void notifyThatIs(const Transaction &t) override {
    std::stringstream stream;
    stream << t.amount;
    std::stringstream dateStream;
    dateStream << t.date;
    view.addTransaction(stream.str(), dateStream.str());
  }
  void notifyThatWillBeRemoved() override {}

private:
  AccountView &view;
};

class AccountPresenter : public Account::Observer {
public:
  explicit AccountPresenter(AccountView &view) : view{view} {}
  void notifyThatBalanceHasChanged(USD) override {}
  void notifyThatAllocationHasChanged(USD) override {}
  void notifyThatHasBeenAdded(ObservableTransaction &t) override {
    transactionPresenters.push_back(
        std::make_unique<TransactionPresenter>(view));
    t.attach(transactionPresenters.back().get());
  }
  void notifyThatWillBeRemoved() override {}

private:
  AccountView &view;
  std::vector<std::unique_ptr<TransactionPresenter>> transactionPresenters;
};
} // namespace sbash64::budget

#endif
