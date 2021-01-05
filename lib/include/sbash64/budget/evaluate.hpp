#ifndef SBASH64_BUDGET_EVALUATE_HPP_
#define SBASH64_BUDGET_EVALUATE_HPP_

#include "budget.hpp"
#include <ostream>
#include <string_view>

namespace sbash64::budget::evaluate {
class Controller {
public:
  void command(Model &, Printer &, std::string_view);

private:
  enum class State { normal, readyForDate, readyForDescription };
  USD amount;
  Date date;
  std::string debitAccountName;
  State state;
  Transaction::Type transactionType;
};

void command(ExpenseRecord &, std::string_view, std::ostream &);
void command(Controller &, Model &, Printer &, std::string_view);
} // namespace sbash64::budget::evaluate

#endif
