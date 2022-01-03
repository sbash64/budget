#include "presentation.hpp"
#include "format.hpp"

#include <sstream>
#include <string>

namespace sbash64::budget {
TransactionPresenter::TransactionPresenter(AccountView &view) : view{view} {}

void TransactionPresenter::notifyThatIsVerified() {}

void TransactionPresenter::notifyThatIsArchived() {}

static auto amount(const Transaction &t) -> std::string {
  std::stringstream stream;
  stream << t.amount;
  return stream.str();
}

static auto date(const Transaction &t) -> std::string {
  std::stringstream stream;
  stream << t.date;
  return stream.str();
}

void TransactionPresenter::notifyThatIs(const Transaction &t) {
  view.addTransaction(amount(t), date(t));
}

void TransactionPresenter::notifyThatWillBeRemoved() {}

AccountPresenter::AccountPresenter(AccountView &view) : view{view} {}

void AccountPresenter::notifyThatBalanceHasChanged(USD) {}

void AccountPresenter::notifyThatAllocationHasChanged(USD) {}

void AccountPresenter::notifyThatHasBeenAdded(ObservableTransaction &t) {
  transactionPresenters.push_back(std::make_unique<TransactionPresenter>(view));
  t.attach(transactionPresenters.back().get());
}

void AccountPresenter::notifyThatWillBeRemoved() {}
} // namespace sbash64::budget
