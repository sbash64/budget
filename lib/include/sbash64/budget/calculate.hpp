#ifndef SBASH64_BUDGET_CALCULATE_HPP_
#define SBASH64_BUDGET_CALCULATE_HPP_

#include "budget.hpp"
#include <vector>

namespace sbash64::budget::calculate {
auto difference(Income, const Expenses &) -> USD;

auto total(const Category &, const Expenses &) -> USD;

auto total(const Expenses &) -> USD;

struct Categories {
  std::vector<Category> each;
};

auto categories(const Expenses &) -> Categories;
} // namespace sbash64::budget::calculate

#endif
