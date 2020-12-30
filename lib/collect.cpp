#include "collect.hpp"

namespace sbash64::budget::collect {
static auto containsCategory(const ExpenseTree &expenseTree,
                             const RecursiveExpense &expense) -> bool {
  return expenseTree.categorizedExpenseTreesOrTotalUsds.count(
             expense.category) == 0;
}

static void insert(ExpenseTree &expenseTree, const RecursiveExpense &expense) {
  if (std::holds_alternative<USD>(expense.subexpenseOrUsd)) {
    if (containsCategory(expenseTree, expense))
      expenseTree.categorizedExpenseTreesOrTotalUsds[expense.category] = USD{0};
    expenseTree.categorizedExpenseTreesOrTotalUsds.at(expense.category) =
        std::get<USD>(expenseTree.categorizedExpenseTreesOrTotalUsds.at(
            expense.category)) +
        std::get<USD>(expense.subexpenseOrUsd);
  } else {
    if (containsCategory(expenseTree, expense))
      expenseTree.categorizedExpenseTreesOrTotalUsds[expense.category] =
          ExpenseTree{};
    insert(
        std::get<ExpenseTree>(expenseTree.categorizedExpenseTreesOrTotalUsds.at(
            expense.category)),
        std::get<Subexpense>(expense.subexpenseOrUsd));
  }
}

auto expenseTree(const std::vector<RecursiveExpense> &expenses) -> ExpenseTree {
  ExpenseTree expenseTree;
  for (const auto &expense : expenses)
    insert(expenseTree, expense);
  return expenseTree;
}
} // namespace sbash64::budget::collect
