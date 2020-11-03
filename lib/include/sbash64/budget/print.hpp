#ifndef SBASH64_BUDGET_PRINT_HPP_
#define SBASH64_BUDGET_PRINT_HPP_

#include "budget.hpp"
#include <ostream>
#include <string>

namespace sbash64 {
namespace budget {
namespace print {
void pretty(std::ostream &, Income, const Expenses &);
auto format(USD) -> std::string;
} // namespace print
} // namespace budget
} // namespace sbash64

#endif
