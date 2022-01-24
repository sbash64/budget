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
                                   BudgetPresenter *parent)
    : view{view}, name_{name}, parent{parent} {
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

static auto placement(
    const std::vector<std::unique_ptr<TransactionPresenter>> &orderedChildren,
    const TransactionPresenter *child) -> gsl::index {
  return distance(orderedChildren.begin(), upperBound(orderedChildren, child));
}

void AccountPresenter::ready(TransactionPresenter *child) {
  const auto placement{budget::placement(orderedChildren, child)};
  view.addTransactionRow(format(child->get().amount), date(child->get()),
                         child->get().description, placement);
  const auto unorderedChild{
      find_if(unorderedChildren.begin(), unorderedChildren.end(),
              [child](const std::unique_ptr<TransactionPresenter> &a) {
                return a.get() == child;
              })};
  orderedChildren.insert(next(orderedChildren.begin(), placement),
                         move(*unorderedChild));
  for (auto i{placement}; i < orderedChildren.size(); ++i)
    orderedChildren.at(i)->setIndex(i);
  unorderedChildren.erase(unorderedChild);
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

void AccountPresenter::notifyThatWillBeRemoved() { parent->remove(this); }

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
