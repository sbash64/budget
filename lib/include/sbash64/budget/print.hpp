#ifndef SBASH64_BUDGET_PRINT_HPP_
#define SBASH64_BUDGET_PRINT_HPP_

#include "budget.hpp"
#include <ostream>
#include <string>
#include <vector>

namespace sbash64::budget::print {
void pretty(std::ostream &, Income, const ExpenseTree &);
void pretty(std::ostream &, const std::vector<LabeledExpense> &);
void pretty(std::ostream &, const std::vector<PrintableTransaction> &);
void pretty(std::ostream &, const LabeledExpense &);
auto format(USD) -> std::string;
} // namespace sbash64::budget::print

#endif
