#ifndef SBASH64_BUDGET_FILE_HPP_
#define SBASH64_BUDGET_FILE_HPP_

#include "budget.hpp"
#include <ostream>

namespace sbash64::budget {
class File : public PersistentMemory {
public:
  explicit File(std::ostream &);
  void save(Account &primary,
            const std::vector<Account *> &secondaries) override;
  void saveAccount(std::string_view name,
                   const std::vector<Transaction> &credits,
                   const std::vector<Transaction> &debits) override;

private:
  std::ostream &stream;
};
} // namespace sbash64::budget

#endif
