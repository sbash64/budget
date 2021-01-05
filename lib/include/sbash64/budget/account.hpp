#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "budget.hpp"
#include <string>
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public Account {
public:
  InMemoryAccount(std::string name);
  void credit(const Transaction &) override;
  void debit(const Transaction &) override;
  void print(Printer &) override;

private:
  std::vector<Transaction> debits;
  std::vector<Transaction> credits;
  std::string name;
};
} // namespace sbash64::budget

#endif
