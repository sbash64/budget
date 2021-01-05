#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "budget.hpp"
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public Account {
public:
  void credit(const Transaction &) override;
  void debit(const Transaction &) override;
  void print(Printer &);

private:
  std::vector<Transaction> debits;
  std::vector<Transaction> credits;
};
} // namespace sbash64::budget

#endif
