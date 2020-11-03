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

struct Category {
  std::string name;
};

struct Expense {
  USD usd{};
  Category category;
};

struct Expenses {
  std::vector<Expense> all;
};

auto difference(Income, const Expenses &) -> USD;

auto total(Category, const Expenses &) -> USD;
} // namespace calculate
} // namespace budget
} // namespace sbash64

#endif
