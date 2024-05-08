#include "presentation.hpp"
#include "domain.hpp"
#include "format.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

namespace sbash64::budget {
TransactionPresenter::TransactionPresenter(ObservableTransaction &transaction,
                                           const std::set<View *> &views,
                                           AccountPresenter &parent)
    : views{views}, parent{parent} {
  transaction.attach(this);
}

void TransactionPresenter::notifyThatIsVerified() {
  verified = true;
  for (const auto &view : views)
    view->putCheckmarkNextToTransactionRow(parent.index(), index);
}

void TransactionPresenter::notifyThatIsArchived() {
  archived = true;
  for (const auto &view : views)
    view->removeTransactionRowSelection(parent.index(), index);
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
  for (const auto &view : views)
    view->deleteTransactionRow(parent.index(), index);
  parent.remove(this);
}

void TransactionPresenter::catchUp(View *view) {
  if (verified)
    view->putCheckmarkNextToTransactionRow(parent.index(), index);
  if (archived)
    view->removeTransactionRowSelection(parent.index(), index);
}

AccountPresenter::AccountPresenter(Account &account,
                                   const std::set<View *> &views,
                                   std::string_view name, Parent &parent)
    : name_{name}, views{views}, parent{parent} {
  account.attach(this);
}

void AccountPresenter::notifyThatBalanceHasChanged(USD usd) {
  balance = usd;
  for (const auto &view : views)
    view->updateAccountBalance(index_, format(usd));
}

void AccountPresenter::notifyThatAllocationHasChanged(USD usd) {
  allocation = usd;
  for (const auto &view : views)
    view->updateAccountAllocation(index_, format(usd));
}

void AccountPresenter::notifyThatHasBeenAdded(ObservableTransaction &t) {
  unorderedChildren.push_back(
      std::make_unique<TransactionPresenter>(t, views, *this));
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
                         std::move(*unorderedChild));
  auto i{childIndex};
  auto it{std::next(orderedChildren.begin(), i)};
  for (; it != orderedChildren.end(); ++it, ++i)
    (*it)->setIndex(i);
  unorderedChildren.erase(unorderedChild);
  for (const auto &view : views)
    view->addTransactionRow(index_, format(child->get().amount),
                            date(child->get()), child->get().description,
                            childIndex);
}

void AccountPresenter::remove(const TransactionPresenter *child) {
  const auto orderedChild{
      find_if(orderedChildren.begin(), orderedChildren.end(),
              [child](const std::unique_ptr<TransactionPresenter> &a) {
                return a.get() == child;
              })};
  auto i{distance(orderedChildren.begin(), next(orderedChild))};
  auto it{std::next(orderedChildren.begin(), i)};
  for (; it != orderedChildren.end(); ++it, ++i)
    (*it)->setIndex(i - 1);
  orderedChildren.erase(orderedChild);
}

void AccountPresenter::notifyThatWillBeRemoved() {
  for (const auto &view : views)
    view->deleteAccountTable(index_);
  parent.remove(this);
}

void AccountPresenter::catchUp(View *view) {
  view->updateAccountBalance(index_, format(balance));
  view->updateAccountAllocation(index_, format(allocation));
  gsl::index i{0};
  for (const auto &child : orderedChildren) {
    view->addTransactionRow(index_, format(child->get().amount),
                            date(child->get()), child->get().description, i++);
    child->catchUp(view);
  }
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

BudgetPresenter::BudgetPresenter(Account &incomeAccount)
    : incomeAccountPresenter{incomeAccount, views, incomeAccountName, *this} {
  incomeAccountPresenter.setIndex(0);
}

void BudgetPresenter::notifyThatExpenseAccountHasBeenCreated(
    Account &account, std::string_view name) {
  const auto childIndex{budget::newChildIndex(orderedChildren, name)};
  orderedChildren.insert(
      next(orderedChildren.begin(), childIndex),
      std::make_unique<AccountPresenter>(account, views, name, *this));
  auto i{childIndex};
  auto it{std::next(orderedChildren.begin(), i)};
  for (; it != orderedChildren.end(); ++it, ++i)
    (*it)->setIndex(i + 1);
  for (const auto &view : views)
    view->addNewAccountTable(name, childIndex + 1);
}

void BudgetPresenter::notifyThatNetIncomeHasChanged(USD usd) {
  netIncome = usd;
  for (const auto &view : views)
    view->updateNetIncome(format(usd));
}

void BudgetPresenter::remove(const AccountPresenter *child) {
  const auto orderedChild{
      find_if(orderedChildren.begin(), orderedChildren.end(),
              [child](const std::unique_ptr<AccountPresenter> &a) {
                return a.get() == child;
              })};
  auto i{distance(orderedChildren.begin(), next(orderedChild))};
  auto it{std::next(orderedChildren.begin(), i)};
  for (; it != orderedChildren.end(); ++it, ++i)
    (*it)->setIndex(i);
  orderedChildren.erase(orderedChild);
}

void BudgetPresenter::notifyThatHasBeenSaved() {
  for (const auto &view : views)
    view->markAsSaved();
}

void BudgetPresenter::notifyThatHasUnsavedChanges() {
  for (const auto &view : views)
    view->markAsUnsaved();
}

void BudgetPresenter::add(View *view) {
  view->updateNetIncome(format(netIncome));
  view->addNewAccountTable(incomeAccountName, 0);
  incomeAccountPresenter.catchUp(view);
  gsl::index i{0};
  for (const auto &account : orderedChildren) {
    view->addNewAccountTable(account->name(), ++i);
    account->catchUp(view);
  }
  views.insert(view);
}

void BudgetPresenter::remove(View *view) { views.erase(view); }
} // namespace sbash64::budget
