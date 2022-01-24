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
  view.putCheckmarkNextToTransaction(index);
}

void TransactionPresenter::notifyThatIsArchived() {
  view.removeTransactionSelection(index);
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

void TransactionPresenter::notifyThatWillBeRemoved() {
  view.deleteTransaction(index);
  parent.remove(this);
}

AccountPresenter::AccountPresenter(Account &account, AccountView &view,
                                   std::string_view name,
                                   BudgetPresenter *parent,
                                   BudgetView *parentView)
    : view{view}, name_{name}, parent{parent}, parentView{parentView} {
  account.attach(this);
}

void AccountPresenter::notifyThatBalanceHasChanged(USD usd) {
  view.updateBalance(format(usd));
}

void AccountPresenter::notifyThatAllocationHasChanged(USD usd) {
  view.updateAllocation(format(usd));
}

void AccountPresenter::notifyThatHasBeenAdded(ObservableTransaction &t) {
  unorderedChildren.push_back(
      std::make_unique<TransactionPresenter>(t, view, *this));
}

static auto upperBound(
    const std::vector<std::unique_ptr<TransactionPresenter>> &orderedChildren,
    const TransactionPresenter *child)
    -> std::vector<std::unique_ptr<TransactionPresenter>>::const_iterator {
  return upper_bound(orderedChildren.begin(), orderedChildren.end(), child,
                     [](const TransactionPresenter *a,
                        const std::unique_ptr<TransactionPresenter> &b) {
                       if (a->get().date != b->get().date)
                         return !(a->get().date < b->get().date);
                       return a->get().description < b->get().description;
                     });
}

static auto
index(const std::vector<std::unique_ptr<TransactionPresenter>> &orderedChildren,
      const TransactionPresenter *child) -> gsl::index {
  return distance(orderedChildren.begin(), upperBound(orderedChildren, child));
}

void AccountPresenter::ready(const TransactionPresenter *child) {
  const auto childIndex{budget::index(orderedChildren, child)};
  const auto unorderedChild{
      find_if(unorderedChildren.begin(), unorderedChildren.end(),
              [child](const std::unique_ptr<TransactionPresenter> &a) {
                return a.get() == child;
              })};
  orderedChildren.insert(next(orderedChildren.begin(), childIndex),
                         move(*unorderedChild));
  for (auto i{childIndex}; i < orderedChildren.size(); ++i)
    orderedChildren.at(i)->setIndex(i);
  unorderedChildren.erase(unorderedChild);
  view.addTransactionRow(format(child->get().amount), date(child->get()),
                         child->get().description, childIndex);
}

void AccountPresenter::remove(const TransactionPresenter *child) {
  auto orderedChild{
      find_if(orderedChildren.begin(), orderedChildren.end(),
              [child](const std::unique_ptr<TransactionPresenter> &a) {
                return a.get() == child;
              })};
  for (auto i{distance(orderedChildren.begin(), next(orderedChild))};
       i < orderedChildren.size(); ++i)
    orderedChildren.at(i)->setIndex(i - 1);
  orderedChildren.erase(orderedChild);
}

void AccountPresenter::notifyThatWillBeRemoved() {
  parentView->deleteAccountTable(index);
  parent->remove(this);
}

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
index(const std::vector<std::unique_ptr<AccountPresenter>> &orderedChildren,
      std::string_view name) -> gsl::index {
  return distance(orderedChildren.begin(), upperBound(orderedChildren, name));
}

BudgetPresenter::BudgetPresenter(BudgetView &view) : view{view} {}

void BudgetPresenter::notifyThatExpenseAccountHasBeenCreated(
    Account &account, std::string_view name) {
  const auto childIndex{budget::index(orderedChildren, name)};
  orderedChildren.insert(
      next(orderedChildren.begin(), childIndex),
      std::make_unique<AccountPresenter>(
          account, view.addNewAccountTable(name, childIndex + 1), name));
  for (auto i{childIndex}; i < orderedChildren.size(); ++i)
    orderedChildren.at(i)->setIndex(i + 1);
}

void BudgetPresenter::notifyThatNetIncomeHasChanged(USD usd) {
  view.updateNetIncome(format(usd));
}

void BudgetPresenter::remove(const AccountPresenter *child) {
  const auto orderedChild{
      find_if(orderedChildren.begin(), orderedChildren.end(),
              [child](const std::unique_ptr<AccountPresenter> &a) {
                return a.get() == child;
              })};
  for (auto i{distance(orderedChildren.begin(), next(orderedChild))};
       i < orderedChildren.size(); ++i)
    orderedChildren.at(i)->setIndex(i);
  orderedChildren.erase(orderedChild);
}
} // namespace sbash64::budget
