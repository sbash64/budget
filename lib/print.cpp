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

static void recursive(std::ostream &stream, const ExpenseTree &expenseTree,
                      int &indentation) {
  stream << format_(calculate::total(expenseTree)) << '\n';
  indentation += 4;
  for (const auto &[category, nextExpenseTreeOrCost] :
       expenseTree.expenseTreeOrUsd) {
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
  stream << "Difference: " << format_(calculate::surplus(income, expenseTree));
}

void pretty(std::ostream &stream, const RecursiveExpense &expense) {
  if (std::holds_alternative<USD>(expense.subexpenseOrUsd)) {
    stream << expense.category.name << ": ";
    stream << format_(std::get<USD>(expense.subexpenseOrUsd));
  } else {
    stream << expense.category.name << "::";
    pretty(stream, std::get<Subexpense>(expense.subexpenseOrUsd));
  }
}

void pretty(std::ostream &stream, const LabeledExpense &expense) {
  pretty(stream, expense.expense);
  stream << " - " << expense.label;
}

void pretty(std::ostream &stream, const std::vector<LabeledExpense> &expenses) {
  bool first{true};
  for (auto expense : expenses) {
    if (!first)
      stream << '\n';
    pretty(stream, expense);
    first = false;
  }
}
} // namespace sbash64::budget::print