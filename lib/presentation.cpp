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
  view.putCheckmarkNextToTransaction(parent.index(this));
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
  parent.ready(this);
}

void TransactionPresenter::notifyThatWillBeRemoved() { parent.remove(this); }

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

static auto
upperBound(const std::vector<const TransactionPresenter *> &transactions,
           const Transaction &t)
    -> std::vector<const TransactionPresenter *>::const_iterator {
  return upper_bound(transactions.begin(), transactions.end(), t,
                     [](const Transaction &a, const TransactionPresenter *b) {
                       if (a.date != b->get().date)
                         return !(a.date < b->get().date);
                       return a.description < b->get().description;
                     });
}

static auto
placement(const std::vector<const TransactionPresenter *> &transactions,
          const Transaction &transaction) -> gsl::index {
  return distance(transactions.begin(), upperBound(transactions, transaction));
}

void AccountPresenter::ready(const TransactionPresenter *child) {
  view.addTransaction(format(child->get().amount), date(child->get()),
                      child->get().description,
                      budget::placement(transactions, child->get()));
  transactions.insert(upperBound(transactions, child->get()), child);
}

auto AccountPresenter::index(const TransactionPresenter *transaction)
    -> gsl::index {
  return distance(transactions.begin(),
                  find(transactions.begin(), transactions.end(), transaction));
}

void AccountPresenter::remove(const TransactionPresenter *t) {
  view.deleteTransaction(index(t));
  transactions.erase(find(transactions.begin(), transactions.end(), t));
}

void AccountPresenter::notifyThatWillBeRemoved() {}
} // namespace sbash64::budget
