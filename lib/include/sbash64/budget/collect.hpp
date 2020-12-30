#ifndef SBASH64_BUDGET_COLLECT_HPP_
#define SBASH64_BUDGET_COLLECT_HPP_

#include "budget.hpp"
#include <vector>

namespace sbash64::budget::collect {
auto expenseTree(const std::vector<RecursiveExpense> &) -> ExpenseTree;
} // namespace sbash64::budget::collect

#endif
