#ifndef SBASH64_BUDGET_TRANSFERRING_TO_NEW_ACCOUNT_MODEL_HPP_
#define SBASH64_BUDGET_TRANSFERRING_TO_NEW_ACCOUNT_MODEL_HPP_

#include <sbash64/budget/budget.hpp>
#include <string_view>

namespace sbash64::budget::transferring_to_new_account {
class Model {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Model);
  virtual void to(std::string_view accountName, USD, Date) = 0;
};
} // namespace sbash64::budget::transferring_to_new_account

#endif
