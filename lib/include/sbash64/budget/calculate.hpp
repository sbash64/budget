#ifndef SBASH64_BUDGET_CALCULATE_HPP_
#define SBASH64_BUDGET_CALCULATE_HPP_

#include "budget.hpp"
#include <vector>

namespace sbash64::budget::calculate {
auto surplus(Income, const ExpenseTree &) -> USD;

auto surplus(Income, const ExpenseTree &, const RecursiveExpense &) -> USD;

auto total(const ExpenseTree &) -> USD;

auto total(const ExpenseTree &, const ExpenseCategory &) -> USD;

auto total(const ExpenseTree &, const RecursiveExpenseCategory &) -> USD;
} // namespace sbash64::budget::calculate

#endif
