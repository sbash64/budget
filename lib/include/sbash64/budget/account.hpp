#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "budget.hpp"
#include <string>
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public Account {
public:
  explicit InMemoryAccount(std::string name);
  void credit(const Transaction &) override;
  void debit(const Transaction &) override;
  void show(View &) override;
  void save(OutputPersistentMemory &) override;
  void load(InputPersistentMemory &) override;

  class Factory : public Account::Factory {
  public:
    auto make(std::string_view name) -> std::shared_ptr<Account> override;
  };

private:
  std::vector<Transaction> debits;
  std::vector<Transaction> credits;
  std::string name;
};
} // namespace sbash64::budget

#endif
