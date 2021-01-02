#include "bank.hpp"

namespace sbash64::budget {
Bank::Bank(AccountFactory &factory) : factory{factory} {
  factory.make("master");
}

void Bank::credit(const Transaction &) {}

void Bank::debit(std::string_view accountName, const Transaction &) {}
} // namespace sbash64::budget