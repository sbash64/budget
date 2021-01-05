#include "print.hpp"
#include "calculate.hpp"
#include <cstdio>
#include <string>

namespace sbash64::budget::print {
static auto formatWithoutDollarSign(USD usd) -> std::string {
  char cents[3];
  std::snprintf(cents, sizeof cents, "%.2lld", usd.cents % 100);
  return std::to_string(usd.cents / 100) + '.' + std::string{cents};
}

static auto format_(USD usd) -> std::string {
  return '$' + formatWithoutDollarSign(usd);
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

static void pretty(std::ostream &stream, const RecursiveExpense &expense) {
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
  for (const auto &expense : expenses) {
    if (!first)
      stream << '\n';
    pretty(stream, expense);
    first = false;
  }
}

static auto format(const Date &date) -> std::string {
  char month[3];
  char day[3];
  std::snprintf(month, sizeof month, "%.2d", date.month);
  std::snprintf(day, sizeof day, "%.2d", date.day);
  return std::string{month} + '/' + std::string{day} + '/' +
         std::to_string(date.year);
}

void pretty(std::ostream &stream,
            const std::vector<PrintableTransaction> &transactions) {
  stream << "Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description";
  for (const auto &transaction : transactions) {
    stream << '\n'
           << formatWithoutDollarSign(transaction.transaction.amount)
           << "                    " << format(transaction.transaction.date)
           << "          " << transaction.transaction.description;
  }
}

StreamPrinter::StreamPrinter(std::ostream &stream) : stream{stream} {}

void StreamPrinter::print(Account &primary,
                          const std::vector<Account *> &secondaries) {
  primary.print(*this);
  for (auto *account : secondaries) {
    stream << "\n\n";
    account->print(*this);
  }
}

void StreamPrinter::printAccountSummary(
    USD balance, const std::vector<PrintableTransaction> &transactions) {}
} // namespace sbash64::budget::print
