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
    -> RecursiveCategory {
  return RecursiveCategory{
      recursiveExpense.category.name,
      std::holds_alternative<Subexpense>(recursiveExpense.subexpenseOrUsd)
          ? Subcategory{recursiveCategory(
                std::get<Subexpense>(recursiveExpense.subexpenseOrUsd))}
          : std::optional<Subcategory>{}};
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

auto surplusAfterUpdate(Income income, const ExpenseTree &expenseTree,
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
                   const RecursiveCategory &recursiveCategory) -> USD;

static auto total_(const ExpenseTree &expenseTree,
                   const RecursiveCategory &category) -> USD {
  if (category.maybeSubcategory.has_value())
    return expenseTree.expenseTreeOrUsd.count(category.category) != 0
               ? total_(expenseTree.expenseTreeOrUsd.at(category.category),
                        category.maybeSubcategory.value())
               : USD{0};
  return total_(expenseTree, category.category);
}

static auto total_(const std::variant<ExpenseTree, USD> &expenseTreeOrUsd,
                   const RecursiveCategory &category) -> USD {
  return std::holds_alternative<ExpenseTree>(expenseTreeOrUsd)
             ? total_(std::get<ExpenseTree>(expenseTreeOrUsd), category)
             : USD{0};
}

auto total(const ExpenseTree &expenseTree, const RecursiveCategory &category)
    -> USD {
  return total_(expenseTree, category);
}

auto total(const ExpenseTree &expenseTree) -> USD {
  return total_(expenseTree);
}
} // namespace sbash64::budget::calculate
