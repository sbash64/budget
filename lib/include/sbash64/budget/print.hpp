#ifndef SBASH64_BUDGET_PRINT_HPP_
#define SBASH64_BUDGET_PRINT_HPP_

#include "budget.hpp"
#include <ostream>
#include <string>
#include <vector>

namespace sbash64::budget::print {
void pretty(std::ostream &, Income, const ExpenseTree &);
void pretty(std::ostream &, const std::vector<LabeledExpense> &);
void pretty(std::ostream &, const std::vector<PrintableTransaction> &);
void pretty(std::ostream &, const LabeledExpense &);
auto format(USD) -> std::string;

class StreamPrinter : public Printer {
public:
  StreamPrinter(std::ostream &);
  void print(Account &primary,
             const std::vector<Account *> &secondaries) override;
  void printAccountSummary(std::string_view name, USD balance,
                           const std::vector<PrintableTransaction> &) override;

private:
  std::ostream &stream;
};
} // namespace sbash64::budget::print

#endif
