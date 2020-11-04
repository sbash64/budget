#include "print.hpp"
#include "calculate.hpp"
#include <cstdio>
#include <string>

namespace sbash64::budget::print {
static auto format_(USD usd) -> std::string {
  char cents[3];
  std::snprintf(cents, sizeof cents, "%.2lld", usd.cents % 100);
  return '$' + std::to_string(usd.cents / 100) + '.' + std::string{cents};
}

auto format(USD usd) -> std::string { return format_(usd); }

void pretty(std::ostream &stream, Income income, const Expenses &expenses) {
  stream << "Income: " << format_(income.usd) << "\n";
  stream << "Expenses: " << format_(calculate::total(expenses)) << "\n";
  for (const auto &category : calculate::categories(expenses).each) {
    stream << "    " << category.name << ": "
           << format_(calculate::total(category, expenses)) << "\n";
  }
  stream << "Difference: " << format_(calculate::difference(income, expenses));
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

static void recursive(std::ostream &stream,
                      const ExpenseTreeWithTotals &expenseTreeWithTotals,
                      int &indentation) {
  stream << format_(expenseTreeWithTotals.totalUsd) << '\n';
  indentation += 4;
  for (const auto &[category, nextExpenseTreeWithTotals] :
       expenseTreeWithTotals.categorizedExpenseTreesWithTotals) {
    stream << std::string(indentation, ' ') << category.name << ": ";
    recursive(stream, nextExpenseTreeWithTotals, indentation);
  }
  indentation -= 4;
}

void pretty(std::ostream &stream, Income income,
            const ExpenseTree &expenseTree) {
  ExpenseTreeWithTotals expenseTreeWithTotals;
  recursivePopulate(expenseTreeWithTotals, expenseTree);
  stream << "Income: " << format_(income.usd) << "\n";
  stream << "Expenses: ";
  int indentation{};
  recursive(stream, expenseTreeWithTotals, indentation);
  stream << "Difference: "
         << format_(income.usd - expenseTreeWithTotals.totalUsd);
}
} // namespace sbash64::budget::print