#include "presentation.hpp"
#include "format.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

namespace sbash64::budget {
TransactionPresenter::TransactionPresenter(AccountView &view,
                                           AccountPresenter &parent)
    : view{view}, parent{parent} {}

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
  parent.notifyThatIs(this, t);
}

void TransactionPresenter::notifyThatWillBeRemoved() {}

AccountPresenter::AccountPresenter(AccountView &view) : view{view} {}

void AccountPresenter::notifyThatBalanceHasChanged(USD) {}

void AccountPresenter::notifyThatAllocationHasChanged(USD) {}

void AccountPresenter::notifyThatHasBeenAdded(ObservableTransaction &t) {
  transactionPresenters.push_back(
      std::make_unique<TransactionPresenter>(view, *this));
  t.attach(transactionPresenters.back().get());
}

void AccountPresenter::notifyThatIs(const TransactionPresenter *child,
                                    const Transaction &t) {
  const auto position{std::upper_bound(transactionDates.begin(),
                                       transactionDates.end(), t.date)};
  view.addTransaction(amount(t), date(t), t.description,
                      std::distance(position, transactionDates.end()));
  transactionDates.insert(position, t.date);
}

void AccountPresenter::notifyThatWillBeRemoved() {}
} // namespace sbash64::budget
