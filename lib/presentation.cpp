#include "presentation.hpp"
#include "domain.hpp"
#include "format.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

namespace sbash64::budget {
TransactionPresenter::TransactionPresenter(ObservableTransaction &transaction,
                                           AccountView &view,
                                           AccountPresenter &parent)
    : view{view}, parent{parent} {
  transaction.attach(this);
}

void TransactionPresenter::notifyThatIsVerified() {
  view.putCheckmarkNextToTransaction(parent.index(this));
}

void TransactionPresenter::notifyThatIsArchived() {
  view.removeTransactionSelection(parent.index(this));
}

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
  auto child{std::make_unique<TransactionPresenter>(t, view, *this)};
  childrenMemory.push_back(move(child));
}

static auto
upperBound(const std::vector<const TransactionPresenter *> &orderedChildren,
           const TransactionPresenter *child)
    -> std::vector<const TransactionPresenter *>::const_iterator {
  return upper_bound(
      orderedChildren.begin(), orderedChildren.end(), child,
      [](const TransactionPresenter *a, const TransactionPresenter *b) {
        if (a->get().date != b->get().date)
          return !(a->get().date < b->get().date);
        return a->get().description < b->get().description;
      });
}

static auto
placement(const std::vector<const TransactionPresenter *> &orderedChildren,
          const TransactionPresenter *child) -> gsl::index {
  return distance(orderedChildren.begin(), upperBound(orderedChildren, child));
}

void AccountPresenter::ready(const TransactionPresenter *child) {
  view.addTransactionRow(format(child->get().amount), date(child->get()),
                         child->get().description,
                         budget::placement(orderedChildren, child));
  orderedChildren.insert(upperBound(orderedChildren, child), child);
}

auto AccountPresenter::index(const TransactionPresenter *child) -> gsl::index {
  return distance(orderedChildren.begin(),
                  find(orderedChildren.begin(), orderedChildren.end(), child));
}

void AccountPresenter::remove(const TransactionPresenter *child) {
  view.deleteTransaction(index(child));
  orderedChildren.erase(
      find(orderedChildren.begin(), orderedChildren.end(), child));
  childrenMemory.erase(
      find_if(childrenMemory.begin(), childrenMemory.end(),
              [child](const std::unique_ptr<TransactionPresenter> &a) {
                return a.get() == child;
              }));
}

void AccountPresenter::notifyThatWillBeRemoved() {}
} // namespace sbash64::budget
