#ifndef SBASH64_BUDGET_TRANSFORM_HPP_
#define SBASH64_BUDGET_TRANSFORM_HPP_

#include "budget.hpp"
#include <vector>

namespace sbash64::budget::transform {
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
} // namespace sbash64::budget::transform

#endif
