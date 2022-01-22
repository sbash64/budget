#include "presentation.hpp"
#include "domain.hpp"
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
  view.putCheckmarkNextToTransaction(parent.index(transaction));
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
  transaction = t;
  parent.notifyThatIs(this, t);
}

void TransactionPresenter::notifyThatWillBeRemoved() {
  parent.remove(transaction);
}

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

static auto upperBound(const std::vector<Transaction> &transactions,
                       const Transaction &t)
    -> std::vector<Transaction>::const_iterator {
  return upper_bound(transactions.begin(), transactions.end(), t,
                     [](const Transaction &a, const Transaction &b) {
                       if (a.date != b.date)
                         return !(a.date < b.date);
                       return a.description < b.description;
                     });
}

static auto placement(const std::vector<Transaction> &transactions,
                      const Transaction &transaction) -> gsl::index {
  return distance(transactions.begin(), upperBound(transactions, transaction));
}

void AccountPresenter::notifyThatIs(const TransactionPresenter *child,
                                    const Transaction &t) {
  view.addTransaction(format(t.amount), date(t), t.description,
                      budget::placement(transactions, t));
  transactions.insert(upperBound(transactions, t), t);
}

auto AccountPresenter::index(const Transaction &transaction) -> gsl::index {
  return distance(transactions.begin(),
                  find(transactions.begin(), transactions.end(), transaction));
}

void AccountPresenter::remove(const Transaction &t) {
  view.deleteTransaction(index(t));
  transactions.erase(find(transactions.begin(), transactions.end(), t));
}

void AccountPresenter::notifyThatWillBeRemoved() {}
} // namespace sbash64::budget
