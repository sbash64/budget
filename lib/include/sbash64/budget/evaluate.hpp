#ifndef SBASH64_BUDGET_EVALUATE_HPP_
#define SBASH64_BUDGET_EVALUATE_HPP_

#include "budget.hpp"
#include <string>
#include <string_view>

namespace sbash64::budget {
class Prompt : public virtual View {
public:
  virtual void prompt(std::string_view) = 0;
};

class Controller {
public:
  void command(Model &, Prompt &, SessionSerialization &,
               SessionDeserialization &, std::string_view);

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

void command(Controller &, Model &, Prompt &, SessionSerialization &,
             SessionDeserialization &, std::string_view);
} // namespace sbash64::budget

#endif
