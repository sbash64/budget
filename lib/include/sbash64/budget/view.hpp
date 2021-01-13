#ifndef SBASH64_BUDGET_VIEW_HPP_
#define SBASH64_BUDGET_VIEW_HPP_

#include "budget.hpp"
#include "command-line.hpp"
#include <ostream>
#include <string>

namespace sbash64::budget {
auto format(USD) -> std::string;

class StreamView : public CommandLineInterface {
public:
  explicit StreamView(std::ostream &);
  void show(Account &primary,
            const std::vector<Account *> &secondaries) override;
  void showAccountSummary(std::string_view name, USD balance,
                          const std::vector<TransactionWithType> &) override;
  void prompt(std::string_view) override;
  void show(const Transaction &, std::string_view suffix) override;

private:
  std::ostream &stream;
};
} // namespace sbash64::budget

#endif
