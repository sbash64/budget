#ifndef SBASH64_BUDGET_PRINT_HPP_
#define SBASH64_BUDGET_PRINT_HPP_

#include "budget.hpp"
#include <map>
#include <ostream>
#include <string>
#include <variant>

namespace sbash64::budget::print {
struct ExpenseTree {
  std::map<Category, std::variant<ExpenseTree, USD>>
      categorizedExpenseTreesOrCosts;
};

void pretty(std::ostream &, Income, const Expenses &);
void pretty(std::ostream &, Income, const ExpenseTree &);
auto format(USD) -> std::string;
} // namespace sbash64::budget::print

#endif
