#ifndef SBASH64_BUDGET_PRINT_HPP_
#define SBASH64_BUDGET_PRINT_HPP_

#include "budget.hpp"
#include <ostream>
#include <string>

namespace sbash64::budget::print {
void pretty(std::ostream &, Income, const ExpenseTree &);
auto format(USD) -> std::string;
} // namespace sbash64::budget::print

#endif
