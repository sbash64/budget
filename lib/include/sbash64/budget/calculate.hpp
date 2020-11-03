#ifndef SBASH64_BUDGET_CALCULATE_HPP_
#define SBASH64_BUDGET_CALCULATE_HPP_

#include "budget.hpp"
#include <string>
#include <vector>

namespace sbash64 {
namespace budget {
namespace calculate {
struct Income {
  USD usd;
};

struct Expense {
  USD usd;
};

struct Expenses {
  std::vector<Expense> all;
};

struct Category {
  std::string name;
};

auto difference(Income, const Expenses &) -> USD;

auto total(Category, const Expenses &) -> USD;
} // namespace calculate
} // namespace budget
} // namespace sbash64

#endif
