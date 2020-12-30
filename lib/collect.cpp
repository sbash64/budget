#include "collect.hpp"

namespace sbash64::budget::collect {
static auto contains(const ExpenseTree &expenseTree,
                     const RecursiveExpense &expense) -> bool {
  return expenseTree.categorizedExpenseTreesOrCosts.count(expense.category) ==
         0;
}

static void add(ExpenseTree &expenseTree, const RecursiveExpense &expense) {
  if (std::holds_alternative<USD>(expense.subexpenseOrUsd)) {
    if (contains(expenseTree, expense))
      expenseTree.categorizedExpenseTreesOrCosts[expense.category] = USD{0};

    expenseTree.categorizedExpenseTreesOrCosts.at(expense.category) =
        std::get<USD>(
            expenseTree.categorizedExpenseTreesOrCosts.at(expense.category)) +
        std::get<USD>(expense.subexpenseOrUsd);
  } else {
    if (contains(expenseTree, expense))
      expenseTree.categorizedExpenseTreesOrCosts[expense.category] =
          ExpenseTree{};
    add(std::get<ExpenseTree>(
            expenseTree.categorizedExpenseTreesOrCosts.at(expense.category)),
        std::get<Subexpense>(expense.subexpenseOrUsd));
  }
}

auto expenseTree(const std::vector<RecursiveExpense> &expenses) -> ExpenseTree {
  ExpenseTree expenseTree;
  for (const auto &expense : expenses)
    add(expenseTree, expense);
  return expenseTree;
}
} // namespace sbash64::budget::collect
