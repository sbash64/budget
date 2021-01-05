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
  enum class CommandType { transaction, transfer };
  USD amount;
  Date date;
  std::string accountName;
  State state{State::normal};
  CommandType commandType;
  Transaction::Type transactionType;
};

void command(ExpenseRecord &, std::string_view, std::ostream &);
void command(Controller &, Model &, Printer &, std::string_view);
} // namespace sbash64::budget::evaluate

#endif
