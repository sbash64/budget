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
  std::map<Category, ExpenseTreeWithTotals> categorizedExpensesWithTotals;
  USD totalUsd{};
};

static void recursivePopulate(ExpenseTreeWithTotals &treeWithTotals,
                              const ExpenseTree &expenseTree) {
  USD totalUsd{0};
  for (const auto &[category, expensesOrCost] :
       expenseTree.categorizedExpensesOrCost) {
    ExpenseTreeWithTotals nextTreeWithTotals;
    if (std::holds_alternative<ExpenseTree>(expensesOrCost)) {
      recursivePopulate(nextTreeWithTotals,
                        std::get<ExpenseTree>(expensesOrCost));
    } else {
      nextTreeWithTotals.totalUsd = std::get<USD>(expensesOrCost);
    }
    totalUsd = totalUsd + nextTreeWithTotals.totalUsd;
    treeWithTotals.categorizedExpensesWithTotals[category] = nextTreeWithTotals;
  }
  treeWithTotals.totalUsd = totalUsd;
}

static void recursivePrint(std::ostream &stream,
                           const ExpenseTreeWithTotals &expenses,
                           int &indentation) {
  stream << format_(expenses.totalUsd) << '\n';
  indentation += 4;
  for (const auto &[category, subexpenses] :
       expenses.categorizedExpensesWithTotals) {
    stream << std::string(indentation, ' ') << category.name << ": ";
    recursivePrint(stream, subexpenses, indentation);
  }
  indentation -= 4;
}

void pretty(std::ostream &stream, Income income, const ExpenseTree &trie) {
  ExpenseTreeWithTotals withSums;
  recursivePopulate(withSums, trie);
  stream << "Income: " << format_(income.usd) << "\n";
  stream << "Expenses: ";
  int indentation{};
  recursivePrint(stream, withSums, indentation);
  stream << "Difference: " << format_(income.usd - withSums.totalUsd);
}
} // namespace sbash64::budget::print