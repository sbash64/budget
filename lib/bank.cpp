#include "bank.hpp"

namespace sbash64::budget {
Bank::Bank(AccountFactory &factory)
    : factory{factory}, masterAccount{factory.make("master")} {}

void Bank::credit(const Transaction &t) { masterAccount->credit(t); }

void Bank::debit(std::string_view accountName, const Transaction &t) {
  const auto account{factory.make(accountName)};
  account->debit(t);
}
} // namespace sbash64::budget