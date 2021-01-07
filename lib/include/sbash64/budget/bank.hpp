#ifndef SBASH64_BUDGET_BANK_HPP_
#define SBASH64_BUDGET_BANK_HPP_

#include "budget.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string_view>
#include <vector>

namespace sbash64::budget {
class Bank : public Model {
public:
  Bank(AccountFactory &);
  void debit(std::string_view accountName, const Transaction &) override;
  void credit(const Transaction &) override;
  void transferTo(std::string_view accountName, USD amount, Date) override;
  void show(View &) override;
  void save(PersistentMemory &) override;
  void load(PersistentMemory &) override;

private:
  AccountFactory &factory;
  std::shared_ptr<Account> masterAccount;
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
};
} // namespace sbash64::budget

#endif
