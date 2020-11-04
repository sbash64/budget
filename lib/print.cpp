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

struct ExpensesWithSums {
  std::map<Category, ExpensesWithSums> expenses;
  USD usd;
};

static auto operator+(USD a, USD b) -> USD { return USD{a.cents + b.cents}; }

static auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

static void recursivePopulate(ExpensesWithSums &withSums,
                              const ExpenseTrie &trie) {
  USD sum{0};
  for (const auto &[category, variant] : trie.trie) {
    ExpensesWithSums next;
    if (std::holds_alternative<ExpenseTrie>(variant)) {
      recursivePopulate(next, std::get<ExpenseTrie>(variant));
    } else {
      next.usd = std::get<USD>(variant);
    }
    sum = sum + next.usd;
    withSums.expenses[category] = next;
  }
  withSums.usd = sum;
}

static void recursivePrint(std::ostream &stream,
                           const ExpensesWithSums &expenses, int &indentation) {
  stream << format_(expenses.usd) << '\n';
  indentation += 4;
  for (const auto &[category, subexpenses] : expenses.expenses) {
    stream << std::string(indentation, ' ') << category.name << ": ";
    recursivePrint(stream, subexpenses, indentation);
  }
  indentation -= 4;
}

void pretty(std::ostream &stream, Income income, const ExpenseTrie &trie) {
  ExpensesWithSums withSums;
  recursivePopulate(withSums, trie);
  stream << "Income: " << format_(income.usd) << "\n";
  stream << "Expenses: ";
  int indentation{};
  recursivePrint(stream, withSums, indentation);
  stream << "Difference: " << format_(income.usd - withSums.usd);
}
} // namespace sbash64::budget::print