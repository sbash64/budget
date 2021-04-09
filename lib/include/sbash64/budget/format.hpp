#ifndef SBASH64_BUDGET_FORMAT_HPP_
#define SBASH64_BUDGET_FORMAT_HPP_

#include "budget.hpp"
#include <ostream>

namespace sbash64::budget {
auto putWithDollarSign(std::ostream &, USD) -> std::ostream &;
auto operator<<(std::ostream &, USD) -> std::ostream &;
auto operator<<(std::ostream &, const Date &) -> std::ostream &;
auto operator<<(std::ostream &, const Month &) -> std::ostream &;
} // namespace sbash64::budget

#endif
