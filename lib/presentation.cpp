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

AccountPresenter::AccountPresenter(Account &account, AccountView &view,
                                   std::string_view name)
    : view{view}, name_{name} {
  account.attach(this);
}

void AccountPresenter::notifyThatBalanceHasChanged(USD usd) {
  view.updateBalance(format(usd));
}

void AccountPresenter::notifyThatAllocationHasChanged(USD usd) {
  view.updateAllocation(format(usd));
}

void AccountPresenter::notifyThatHasBeenAdded(ObservableTransaction &t) {
  childrenMemory.push_back(
      std::make_unique<TransactionPresenter>(t, view, *this));
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

static auto upperBound(
    const std::vector<std::unique_ptr<AccountPresenter>> &orderedChildren,
    std::string_view name)
    -> std::vector<std::unique_ptr<AccountPresenter>>::const_iterator {
  return upper_bound(
      orderedChildren.begin(), orderedChildren.end(), name,
      [](std::string_view a, const std::unique_ptr<AccountPresenter> &b) {
        return a < b->name();
      });
}

static auto
placement(const std::vector<std::unique_ptr<AccountPresenter>> &orderedChildren,
          std::string_view name) -> gsl::index {
  return distance(orderedChildren.begin(), upperBound(orderedChildren, name));
}

BudgetPresenter::BudgetPresenter(BudgetView &view) : view{view} {}

void BudgetPresenter::notifyThatExpenseAccountHasBeenCreated(
    Account &account, std::string_view name) {
  orderedChildren.insert(
      upperBound(orderedChildren, name),
      std::make_unique<AccountPresenter>(
          account,
          view.addNewAccountTable(name,
                                  budget::placement(orderedChildren, name) + 1),
          name));
}

void BudgetPresenter::notifyThatNetIncomeHasChanged(USD usd) {
  view.updateNetIncome(format(usd));
}

void BudgetPresenter::remove(const AccountPresenter *child) {
  // view.deleteAccountTable(index(child));
  orderedChildren.erase(
      find_if(orderedChildren.begin(), orderedChildren.end(),
              [child](const std::unique_ptr<AccountPresenter> &a) {
                return a.get() == child;
              }));
}
} // namespace sbash64::budget
