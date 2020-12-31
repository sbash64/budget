#ifndef SBASH64_BUDGET_EVALUATE_HPP_
#define SBASH64_BUDGET_EVALUATE_HPP_

#include "budget.hpp"
#include <string_view>

namespace sbash64::budget::evaluate {
void command(ExpenseRecord &, std::string_view);
} // namespace sbash64::budget::evaluate

#endif