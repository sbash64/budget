#include "presentation.hpp"
#include "format.hpp"

#include <sstream>

namespace sbash64::budget {
TransactionPresenter::TransactionPresenter(AccountView &view) : view{view} {}

void TransactionPresenter::notifyThatIsVerified() {}

void TransactionPresenter::notifyThatIsArchived() {}

void TransactionPresenter::notifyThatIs(const Transaction &t) {
  std::stringstream stream;
  stream << t.amount;
  std::stringstream dateStream;
  dateStream << t.date;
  view.addTransaction(stream.str(), dateStream.str());
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
