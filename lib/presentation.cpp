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

void TransactionPresenter::notifyThatIsVerified() {
  view.putCheckmarkNextToTransaction(parent.index(date));
}

void TransactionPresenter::notifyThatIsArchived() {}

static auto format(USD usd) -> std::string {
  std::stringstream stream;
  stream << usd;
  return stream.str();
}

static auto date(const Transaction &t) -> std::string {
  std::stringstream stream;
  stream << t.date;
  return stream.str();
}

void TransactionPresenter::notifyThatIs(const Transaction &t) {
  date = t.date;
  parent.notifyThatIs(this, t);
}

void TransactionPresenter::notifyThatWillBeRemoved() {}

AccountPresenter::AccountPresenter(AccountView &view) : view{view} {}

void AccountPresenter::notifyThatBalanceHasChanged(USD usd) {
  view.updateBalance(format(usd));
}

void AccountPresenter::notifyThatAllocationHasChanged(USD usd) {
  view.updateAllocation(format(usd));
}

void AccountPresenter::notifyThatHasBeenAdded(ObservableTransaction &t) {
  transactionPresenters.push_back(
      std::make_unique<TransactionPresenter>(view, *this));
  t.attach(transactionPresenters.back().get());
}

void AccountPresenter::notifyThatIs(const TransactionPresenter *child,
                                    const Transaction &t) {
  const auto position{
      upper_bound(transactionDates.begin(), transactionDates.end(), t.date)};
  view.addTransaction(format(t.amount), date(t), t.description,
                      distance(position, transactionDates.end()));
  transactionDates.insert(position, t.date);
}

auto AccountPresenter::index(const Date &date) -> int {
  return distance(
      upper_bound(transactionDates.begin(), transactionDates.end(), date),
      transactionDates.end());
}

void AccountPresenter::notifyThatWillBeRemoved() {}
} // namespace sbash64::budget
