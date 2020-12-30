#ifndef SBASH64_BUDGET_EVALUATE_HPP_
#define SBASH64_BUDGET_EVALUATE_HPP_

#include "budget.hpp"
#include <string_view>

namespace sbash64::budget::evaluate {
void command(ExpenseRecord &record, std::string_view s) {
  record.enter(LabeledExpense{
      RecursiveExpense{ExpenseCategory{"Gifts"},
                       Subexpense{RecursiveExpense{ExpenseCategory{"Birthdays"},
                                                   USD{2500}}}},
      "Sam's 24th"});
}
} // namespace sbash64::budget::evaluate

#endif
