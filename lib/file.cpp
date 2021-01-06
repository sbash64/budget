#include "file.hpp"

namespace sbash64::budget {
void File::save(Account &primary, const std::vector<Account *> &secondaries) {
  primary.save(*this);
  for (auto *account : secondaries)
    account->save(*this);
}

void File::saveAccount(std::string_view name,
                       const std::vector<Transaction> &credits,
                       const std::vector<Transaction> &debits) {}
} // namespace sbash64::budget
