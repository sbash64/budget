#ifndef SBASH64_BUDGET_BANK_HPP_
#define SBASH64_BUDGET_BANK_HPP_

#include "budget.hpp"
#include <map>
#include <memory>
#include <string_view>

namespace sbash64::budget {
class Account {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Account);
  virtual void credit(const Transaction &) = 0;
  virtual void debit(const Transaction &) = 0;
};

class AccountFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountFactory);
  virtual auto make(std::string_view name) -> std::shared_ptr<Account> = 0;
};

class Bank : public Model {
public:
  Bank(AccountFactory &);
  void debit(std::string_view accountName, const Transaction &) override;
  void credit(const Transaction &) override;

private:
  AccountFactory &factory;
  std::shared_ptr<Account> masterAccount;
  std::map<std::string, std::shared_ptr<Account>> accounts;
};
} // namespace sbash64::budget

#endif
