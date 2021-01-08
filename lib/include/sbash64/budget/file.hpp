#ifndef SBASH64_BUDGET_FILE_HPP_
#define SBASH64_BUDGET_FILE_HPP_

#include "budget.hpp"
#include <istream>
#include <ostream>

namespace sbash64::budget {
class PersistentStreams : public PersistentMemory {
public:
  explicit PersistentStreams(std::istream &, std::ostream &);
  void save(Account &primary,
            const std::vector<Account *> &secondaries) override;
  void saveAccount(std::string_view name,
                   const std::vector<Transaction> &credits,
                   const std::vector<Transaction> &debits) override;
  void loadAccount(std::vector<Transaction> &credits,
                   std::vector<Transaction> &debits) override;
  void load(Account::Factory &factory, std::shared_ptr<Account> &primary,
            std::map<std::string, std::shared_ptr<Account>, std::less<>>
                &secondaries) override;

private:
  std::istream &input;
  std::ostream &output;
};
} // namespace sbash64::budget

#endif
