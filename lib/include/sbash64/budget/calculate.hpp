#ifndef SBASH64_BUDGET_CALCULATE_HPP_
#define SBASH64_BUDGET_CALCULATE_HPP_

#include "budget.hpp"

namespace sbash64 {
namespace budget {
namespace calculate {
auto difference(Income, const Expenses &) -> USD;

auto total(const Category &, const Expenses &) -> USD;

auto total(const Expenses &) -> USD;
} // namespace calculate
} // namespace budget
} // namespace sbash64

#endif
