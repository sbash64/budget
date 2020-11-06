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

static void recursive(std::ostream &stream, const ExpenseTree &expenseTree,
                      int &indentation) {
  stream << format_(calculate::total(expenseTree)) << '\n';
  indentation += 4;
  for (const auto &[category, nextExpenseTreeOrCost] :
       expenseTree.categorizedExpenseTreesOrCosts) {
    stream << std::string(indentation, ' ') << category.name << ": ";
    if (std::holds_alternative<ExpenseTree>(nextExpenseTreeOrCost))
      recursive(stream, std::get<ExpenseTree>(nextExpenseTreeOrCost),
                indentation);
    else
      stream << format_(std::get<USD>(nextExpenseTreeOrCost)) << '\n';
  }
  indentation -= 4;
}

void pretty(std::ostream &stream, Income income,
            const ExpenseTree &expenseTree) {
  stream << "Income: " << format_(income.usd) << "\n";
  stream << "Expenses: ";
  int indentation{};
  recursive(stream, expenseTree, indentation);
  stream << "Difference: "
         << format_(calculate::difference(income, expenseTree));
}
} // namespace sbash64::budget::print