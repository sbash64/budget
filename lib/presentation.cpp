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

void TransactionPresenter::notifyThatIs(const Transaction &t) {
  std::stringstream dateStream;
  dateStream << t.date;
  view.addTransaction(amount(t), dateStream.str());
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
