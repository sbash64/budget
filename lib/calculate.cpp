#include "calculate.hpp"
#include <numeric>
#include <set>

namespace sbash64::budget::calculate {
static auto total_(const Expenses &expenses) -> USD {
  return std::accumulate(
      expenses.all.begin(), expenses.all.end(), USD{0},
      [](USD usd, const Expense &expense) -> USD { return expense.usd + usd; });
}

auto total(const Expenses &expenses) -> USD { return total_(expenses); }

auto difference(Income income, const Expenses &expenses) -> USD {
  return income.usd - total_(expenses);
}

auto total(const Category &category, const Expenses &expenses) -> USD {
  return std::accumulate(expenses.all.begin(), expenses.all.end(), USD{0},
                         [=](USD usd, const Expense &expense) -> USD {
                           return category == expense.category
                                      ? expense.usd + usd
                                      : usd;
                         });
}

struct ExpenseTreeWithTotals {
  std::map<Category, ExpenseTreeWithTotals> categorizedExpenseTreesWithTotals;
  USD totalUsd{};
};

static void recursivePopulate(ExpenseTreeWithTotals &expenseTreeWithTotals,
                              const ExpenseTree &expenseTree) {
  USD totalUsd{0};
  for (const auto &[category, expenseTreeOrCost] :
       expenseTree.categorizedExpenseTreesOrCosts) {
    ExpenseTreeWithTotals nextExpenseTreeWithTotals;
    if (std::holds_alternative<ExpenseTree>(expenseTreeOrCost))
      recursivePopulate(nextExpenseTreeWithTotals,
                        std::get<ExpenseTree>(expenseTreeOrCost));
    else
      nextExpenseTreeWithTotals.totalUsd = std::get<USD>(expenseTreeOrCost);
    totalUsd = totalUsd + nextExpenseTreeWithTotals.totalUsd;
    expenseTreeWithTotals.categorizedExpenseTreesWithTotals[category] =
        nextExpenseTreeWithTotals;
  }
  expenseTreeWithTotals.totalUsd = totalUsd;
}

auto total_(const ExpenseTree &expenseTree, const Category &category) -> USD {
  if (std::holds_alternative<ExpenseTree>(
          expenseTree.categorizedExpenseTreesOrCosts.at(category))) {
    ExpenseTreeWithTotals expenseTreeWithTotals;
    recursivePopulate(
        expenseTreeWithTotals,
        std::get<ExpenseTree>(
            expenseTree.categorizedExpenseTreesOrCosts.at(category)));
    return expenseTreeWithTotals.totalUsd;
  }
  return std::get<USD>(expenseTree.categorizedExpenseTreesOrCosts.at(category));
}

auto total(const ExpenseTree &expenseTree, const Category &category) -> USD {
  return total_(expenseTree, category);
}

auto total_(const ExpenseTree &expenseTree, const RecursiveCategory &category)
    -> USD {
  return category.maybeSubcategory.has_value()
             ? total_(
                   std::get<ExpenseTree>(
                       expenseTree.categorizedExpenseTreesOrCosts.at(category)),
                   category.maybeSubcategory.value())
             : total_(expenseTree, static_cast<const Category &>(category));
}

auto total(const ExpenseTree &expenseTree, const RecursiveCategory &category)
    -> USD {
  return total_(expenseTree, category);
}

auto categories(const Expenses &expenses) -> Categories {
  std::set<Category> uniqueCategories;
  std::for_each(expenses.all.begin(), expenses.all.end(),
                [&](const Expense &expense) {
                  uniqueCategories.insert(expense.category);
                });
  return Categories{{uniqueCategories.begin(), uniqueCategories.end()}};
}
} // namespace sbash64::budget::calculate
