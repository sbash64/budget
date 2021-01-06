#ifndef SBASH64_BUDGET_PRINT_HPP_
#define SBASH64_BUDGET_PRINT_HPP_

#include "budget.hpp"
#include <ostream>
#include <string>

namespace sbash64::budget {
auto format(USD) -> std::string;

class StreamPrinter : public View {
public:
  StreamPrinter(std::ostream &);
  void show(Account &primary,
            const std::vector<Account *> &secondaries) override;
  void showAccountSummary(std::string_view name, USD balance,
                          const std::vector<PrintableTransaction> &) override;

private:
  std::ostream &stream;
};
} // namespace sbash64::budget

#endif
