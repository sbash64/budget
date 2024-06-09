#include "presentation.hpp"
#include "domain.hpp"
#include "format.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>
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
    view->putCheckmarkNextToTransactionRow(parent.parent.index(&parent),
                                           parent.index(this));
}

void TransactionPresenter::notifyThatIsArchived() {
  archived = true;
  for (const auto &view : views)
    view->removeTransactionRowSelection(parent.parent.index(&parent),
                                        parent.index(this));
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
    view->deleteTransactionRow(parent.parent.index(&parent),
                               parent.index(this));
  parent.remove(this);
}

void TransactionPresenter::catchUp(View *view) {
  if (verified)
    view->putCheckmarkNextToTransactionRow(parent.parent.index(&parent),
                                           parent.index(this));
  if (archived)
    view->removeTransactionRowSelection(parent.parent.index(&parent),
                                        parent.index(this));
}

static auto operator<(const TransactionPresenter &a,
                      const TransactionPresenter &b) -> bool {
  if (a.get().date != b.get().date)
    return !(a.get().date < b.get().date);
  if (a.get().description != b.get().description)
    return a.get().description < b.get().description;
  if (a.get().amount != b.get().amount)
    return a.get().amount.cents < b.get().amount.cents;
  return &a < &b;
}

static auto operator<(const std::unique_ptr<TransactionPresenter> &a,
                      const std::unique_ptr<TransactionPresenter> &b) -> bool {
  return *a < *b;
}

static auto operator<(const std::unique_ptr<TransactionPresenter> &a,
                      const TransactionPresenter &b) -> bool {
  return *a < b;
}

static auto operator<(const TransactionPresenter &a,
                      const std::unique_ptr<TransactionPresenter> &b) -> bool {
  return a < *b;
}

static auto transactionIndex(
    const std::set<std::unique_ptr<TransactionPresenter>, std::less<>>
        &orderedChildren,
    std::set<std::unique_ptr<TransactionPresenter>>::const_iterator it)
    -> gsl::index {
  return std::distance(orderedChildren.begin(), it);
}

AccountPresenter::AccountPresenter(Account &account,
                                   const std::set<View *> &views,
                                   std::string_view name, Parent &parent)
    : name{name}, parent{parent}, views{views} {
  account.attach(this);
}

void AccountPresenter::notifyThatNameHasChanged(std::string_view name) {
  parent.reorder(this, name);
  for (const auto &view : views)
    view->setAccountName(parent.index(this), name);
}

void AccountPresenter::notifyThatBalanceHasChanged(USD usd) {
  balance = usd;
  for (const auto &view : views)
    view->updateAccountBalance(parent.index(this), format(usd));
}

void AccountPresenter::notifyThatAllocationHasChanged(USD usd) {
  allocation = usd;
  for (const auto &view : views)
    view->updateAccountAllocation(parent.index(this), format(usd));
}

void AccountPresenter::notifyThatHasBeenAdded(ObservableTransaction &t) {
  unorderedChildren.push_back(
      std::make_unique<TransactionPresenter>(t, views, *this));
}

void AccountPresenter::ready(const TransactionPresenter *child) {
  const auto unorderedChild{
      find_if(unorderedChildren.begin(), unorderedChildren.end(),
              [child](const std::unique_ptr<TransactionPresenter> &a) {
                return a.get() == child;
              })};
  if (unorderedChild == unorderedChildren.end())
    throw std::runtime_error{"Unable to find transaction presenter"};
  const auto [it, success]{orderedChildren.insert(std::move(*unorderedChild))};
  if (!success)
    throw std::runtime_error{"Unable to insert transaction presenter"};
  unorderedChildren.erase(unorderedChild);
  for (const auto &view : views)
    view->addTransactionRow(parent.index(this), format(child->get().amount),
                            date(child->get()), child->get().description,
                            transactionIndex(orderedChildren, it));
}

void AccountPresenter::remove(const TransactionPresenter *child) {
  const auto it = orderedChildren.find(*child);
  if (it == orderedChildren.end())
    throw std::runtime_error{
        "Unable to find transaction presenter for removal"};
  orderedChildren.erase(it);
}

void AccountPresenter::notifyThatWillBeRemoved() {
  for (const auto &view : views)
    view->deleteAccountTable(parent.index(this));
  parent.remove(this);
}

void AccountPresenter::catchUp(View *view) {
  view->updateAccountBalance(parent.index(this), format(balance));
  view->updateAccountAllocation(parent.index(this), format(allocation));
  for (const auto &child : orderedChildren) {
    view->addTransactionRow(parent.index(this), format(child->get().amount),
                            date(child->get()), child->get().description,
                            index(child.get()));
    child->catchUp(view);
  }
}

auto AccountPresenter::index(const TransactionPresenter *child) -> gsl::index {
  return transactionIndex(orderedChildren, orderedChildren.find(*child));
}

static auto operator<(const AccountPresenter &a, const AccountPresenter &b)
    -> bool {
  if (a.name != b.name)
    return a.name < b.name;
  return &a < &b;
}

static auto operator<(const std::unique_ptr<AccountPresenter> &a,
                      const std::unique_ptr<AccountPresenter> &b) -> bool {
  return *a < *b;
}

static auto operator<(const std::unique_ptr<AccountPresenter> &a,
                      const AccountPresenter &b) -> bool {
  return *a < b;
}

static auto operator<(const AccountPresenter &a,
                      const std::unique_ptr<AccountPresenter> &b) -> bool {
  return a < *b;
}

static auto
accountIndex(const std::set<std::unique_ptr<AccountPresenter>, std::less<>>
                 &orderedChildren,
             std::set<std::unique_ptr<AccountPresenter>>::const_iterator it)
    -> gsl::index {
  return std::distance(orderedChildren.begin(), it) + 1;
}

BudgetPresenter::BudgetPresenter(Account &account)
    : incomeAccount{account, views, incomeAccountName, *this} {}

void BudgetPresenter::notifyThatExpenseAccountHasBeenCreated(
    Account &account, std::string_view name) {
  auto [it, success]{accounts.insert(
      std::make_unique<AccountPresenter>(account, views, name, *this))};
  if (!success)
    throw std::runtime_error{"Unable to insert account presenter"};
  for (const auto &view : views)
    view->addNewAccountTable(name, accountIndex(accounts, it));
}

void BudgetPresenter::notifyThatNetIncomeHasChanged(USD usd) {
  netIncome = usd;
  for (const auto &view : views)
    view->updateNetIncome(format(usd));
}

void BudgetPresenter::remove(const AccountPresenter *child) {
  const auto it = accounts.find(*child);
  if (it == accounts.end())
    throw std::runtime_error{"Unable to find account presenter for removal"};
  accounts.erase(it);
}

void BudgetPresenter::reorder(const AccountPresenter *child,
                              std::string_view newName) {
  const auto from = accounts.find(*child);
  if (from == accounts.end())
    throw std::runtime_error{"Unable to find account presenter for reordering"};
  const auto fromIndex{accountIndex(accounts, from)};
  auto node{accounts.extract(from)};
  node.value()->name = newName;
  const auto to{accounts.insert(std::move(node))};
  const auto toIndex{accountIndex(accounts, to.position)};
  for (const auto &view : views)
    view->reorderAccountIndex(fromIndex, toIndex);
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
  incomeAccount.catchUp(view);
  for (const auto &account : accounts) {
    view->addNewAccountTable(account->name, index(account.get()));
    account->catchUp(view);
  }
  views.insert(view);
}

void BudgetPresenter::remove(View *view) { views.erase(view); }

auto BudgetPresenter::index(const AccountPresenter *account) -> gsl::index {
  if (account == &incomeAccount)
    return 0;
  const auto it = accounts.find(*account);
  if (it == accounts.end())
    throw std::runtime_error{"Unable to find account presenter for index"};
  return accountIndex(accounts, it);
}
} // namespace sbash64::budget
