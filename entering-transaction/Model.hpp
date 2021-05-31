#ifndef SBASH64_BUDGET_ENTERING_TRANSACTION_MODEL_HPP_
#define SBASH64_BUDGET_ENTERING_TRANSACTION_MODEL_HPP_

#include <sbash64/budget/budget.hpp>

namespace sbash64::budget::entering_transaction {
class Model {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Model);
  virtual void submit(const Transaction &) = 0;
};
} // namespace sbash64::budget::entering_transaction

#endif
