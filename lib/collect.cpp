#include "collect.hpp"

namespace sbash64::budget::collect {
static void add(ExpenseTree &expenseTree, const RecursiveExpense &expense) {
  if (std::holds_alternative<USD>(expense.subexpenseOrUsd)) {
    if (expenseTree.categorizedExpenseTreesOrCosts.count(expense.category) == 0)
      expenseTree.categorizedExpenseTreesOrCosts[expense.category] = USD{0};

    expenseTree.categorizedExpenseTreesOrCosts[expense.category] =
        std::get<USD>(
            expenseTree.categorizedExpenseTreesOrCosts[expense.category]) +
        std::get<USD>(expense.subexpenseOrUsd);
  } else {
    if (expenseTree.categorizedExpenseTreesOrCosts.count(expense.category) == 0)
      expenseTree.categorizedExpenseTreesOrCosts[expense.category] =
          ExpenseTree{};
    add(std::get<ExpenseTree>(
            expenseTree.categorizedExpenseTreesOrCosts[expense.category]),
        std::get<Subexpense>(expense.subexpenseOrUsd));
  }
}

auto expenseTree(const std::vector<RecursiveExpense> &v) -> ExpenseTree {
  ExpenseTree expenseTree;
  for (auto x : v)
    add(expenseTree, x);
  return expenseTree;
}
} // namespace sbash64::budget::collect
