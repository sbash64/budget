#include "presentation.hpp"
#include "domain.hpp"
#include "format.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

namespace sbash64::budget {
TransactionPresenter::TransactionPresenter(ObservableTransaction &transaction,
                                           View &view, AccountPresenter &parent)
    : view{view}, parent{parent} {
  transaction.attach(this);
}

void TransactionPresenter::notifyThatIsVerified() {
  view.putCheckmarkNextToTransactionRow(index);
}

void TransactionPresenter::notifyThatIsArchived() {
  view.removeTransactionRowSelection(index);
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
  view.deleteTransactionRow(index);
  parent.remove(this);
}

AccountPresenter::AccountPresenter(Account &account, View &view,
                                   std::string_view name,
                                   BudgetPresenter *parent)
    : view{view}, name_{name}, parent{parent} {
  account.attach(this);
}

void AccountPresenter::notifyThatBalanceHasChanged(USD usd) {
  view.updateAccountBalance(format(usd));
}

void AccountPresenter::notifyThatAllocationHasChanged(USD usd) {
  view.updateAccountAllocation(format(usd));
}

void AccountPresenter::notifyThatHasBeenAdded(ObservableTransaction &t) {
  unorderedChildren.push_back(
      std::make_unique<TransactionPresenter>(t, view, *this));
}

static auto newChildIndex(
    const std::vector<std::unique_ptr<TransactionPresenter>> &orderedChildren,
    const TransactionPresenter *child) -> gsl::index {
  return distance(
      orderedChildren.begin(),
      upper_bound(orderedChildren.begin(), orderedChildren.end(), child,
                  [](const TransactionPresenter *a,
                     const std::unique_ptr<TransactionPresenter> &b) {
                    if (a->get().date != b->get().date)
                      return !(a->get().date < b->get().date);
                    return a->get().description < b->get().description;
                  }));
}

void AccountPresenter::ready(const TransactionPresenter *child) {
  const auto childIndex{budget::newChildIndex(orderedChildren, child)};
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
  view.deleteAccountTable(index);
  parent->remove(this);
}

static auto newChildIndex(
    const std::vector<std::unique_ptr<AccountPresenter>> &orderedChildren,
    std::string_view name) -> gsl::index {
  return distance(orderedChildren.begin(),
                  upper_bound(orderedChildren.begin(), orderedChildren.end(),
                              name,
                              [](std::string_view a,
                                 const std::unique_ptr<AccountPresenter> &b) {
                                return a < b->name();
                              }));
}

BudgetPresenter::BudgetPresenter(View &view) : view{view} {}

void BudgetPresenter::notifyThatExpenseAccountHasBeenCreated(
    Account &account, std::string_view name) {
  const auto childIndex{budget::newChildIndex(orderedChildren, name)};
  orderedChildren.insert(
      next(orderedChildren.begin(), childIndex),
      std::make_unique<AccountPresenter>(account, view, name));
  for (auto i{childIndex}; i < orderedChildren.size(); ++i)
    orderedChildren.at(i)->setIndex(i + 1);
  view.addNewAccountTable(name, childIndex + 1);
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
