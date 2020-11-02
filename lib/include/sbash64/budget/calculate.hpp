#ifndef SBASH64_BUDGET_CALCULATE_HPP_
#define SBASH64_BUDGET_CALCULATE_HPP_

#include "budget.hpp"

namespace sbash64 {
namespace budget {
namespace calculate {
struct Income {
  USD usd;
};
struct Expenses {};
auto difference(Income, Expenses) -> USD;
} // namespace calculate
} // namespace budget
} // namespace sbash64

#endif
