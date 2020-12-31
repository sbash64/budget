#include "calculate.hpp"
#include <algorithm>
#include <numeric>

namespace sbash64::budget::calculate {
static auto total_(const ExpenseTree &expenseTree) -> USD {
  USD totalUsd{0};
  for (const auto &[category, expenseTreeOrTotalUsd] :
       expenseTree.expenseTreeOrUsd)
    totalUsd =
        totalUsd + (std::holds_alternative<ExpenseTree>(expenseTreeOrTotalUsd)
                        ? total_(std::get<ExpenseTree>(expenseTreeOrTotalUsd))
                        : std::get<USD>(expenseTreeOrTotalUsd));
  return totalUsd;
}

auto surplus(Income income, const ExpenseTree &expenseTree) -> USD {
  return income.usd - total_(expenseTree);
}

static auto usd(const RecursiveExpense &recursiveExpense) -> USD {
  return std::holds_alternative<Subexpense>(recursiveExpense.subexpenseOrUsd)
             ? usd(std::get<Subexpense>(recursiveExpense.subexpenseOrUsd))
             : std::get<USD>(recursiveExpense.subexpenseOrUsd);
}

static auto recursiveCategory(const RecursiveExpense &recursiveExpense)
    -> RecursiveExpenseCategory {
  return RecursiveExpenseCategory{
      recursiveExpense.category.name,
      std::holds_alternative<Subexpense>(recursiveExpense.subexpenseOrUsd)
          ? ExpenseSubcategory{recursiveCategory(
                std::get<Subexpense>(recursiveExpense.subexpenseOrUsd))}
          : std::optional<ExpenseSubcategory>{}};
}

static auto currentCost(const ExpenseTree &expenseTree,
                        const RecursiveExpense &recursiveExpense) -> USD {
  return std::holds_alternative<Subexpense>(recursiveExpense.subexpenseOrUsd)
             ? currentCost(
                   std::get<ExpenseTree>(expenseTree.expenseTreeOrUsd.at(
                       recursiveExpense.category)),
                   std::get<Subexpense>(recursiveExpense.subexpenseOrUsd))
             : std::get<USD>(
                   expenseTree.expenseTreeOrUsd.at(recursiveExpense.category));
}

auto surplus(Income income, const ExpenseTree &expenseTree,
             const RecursiveExpense &recursiveExpense) -> USD {
  return income.usd -
         (total_(expenseTree) - currentCost(expenseTree, recursiveExpense) +
          usd(recursiveExpense));
}

static auto total_(const std::variant<ExpenseTree, USD> &expenseTreeOrUsd)
    -> USD {
  return std::holds_alternative<ExpenseTree>(expenseTreeOrUsd)
             ? total_(std::get<ExpenseTree>(expenseTreeOrUsd))
             : std::get<USD>(expenseTreeOrUsd);
}

static auto total_(const ExpenseTree &expenseTree, const Category &category)
    -> USD {
  return expenseTree.expenseTreeOrUsd.count(category) == 0
             ? USD{0}
             : total_(expenseTree.expenseTreeOrUsd.at(category));
}

auto total(const ExpenseTree &expenseTree, const Category &category) -> USD {
  return total_(expenseTree, category);
}

static auto total_(const std::variant<ExpenseTree, USD> &expenseTreeOrUsd,
                   const RecursiveExpenseCategory &recursiveCategory) -> USD;

static auto total_(const ExpenseTree &expenseTree,
                   const RecursiveExpenseCategory &recursiveCategory) -> USD {
  if (recursiveCategory.maybeSubcategory.has_value())
    return expenseTree.expenseTreeOrUsd.count(recursiveCategory.category) != 0
               ? total_(expenseTree.expenseTreeOrUsd.at(
                            recursiveCategory.category),
                        recursiveCategory.maybeSubcategory.value())
               : USD{0};
  return total_(expenseTree, recursiveCategory.category);
}

static auto total_(const std::variant<ExpenseTree, USD> &expenseTreeOrUsd,
                   const RecursiveExpenseCategory &recursiveCategory) -> USD {
  return std::holds_alternative<ExpenseTree>(expenseTreeOrUsd)
             ? total_(std::get<ExpenseTree>(expenseTreeOrUsd),
                      recursiveCategory)
             : USD{0};
}

auto total(const ExpenseTree &expenseTree,
           const RecursiveExpenseCategory &recursiveCategory) -> USD {
  return total_(expenseTree, recursiveCategory);
}

auto total(const ExpenseTree &expenseTree) -> USD {
  return total_(expenseTree);
}
} // namespace sbash64::budget::calculate
