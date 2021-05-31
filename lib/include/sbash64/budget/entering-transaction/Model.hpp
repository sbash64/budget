#ifndef SBASH64_BUDGET_ENTERING_TRANSACTION_MODEL_HPP_
#define SBASH64_BUDGET_ENTERING_TRANSACTION_MODEL_HPP_

#include "../budget.hpp"
#include <string_view>

namespace sbash64::budget::entering_transaction {
class Model {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Model);
  virtual void debit(std::string_view accountName, const Transaction &) = 0;
  virtual void credit(const Transaction &) = 0;
};
} // namespace sbash64::budget::entering_transaction

#endif
