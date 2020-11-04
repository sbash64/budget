#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include <cstdint>
#include <string>
#include <vector>

namespace sbash64::budget {
struct USD {
  std::int_least64_t cents;
};

struct Income {
  USD usd;
};

struct Category {
  std::string name;
};

inline auto operator<(const Category &a, const Category &b) -> bool {
  return a.name < b.name;
}

struct Expense {
  USD usd{};
  Category category;
};

struct Expenses {
  std::vector<Expense> all;
};
} // namespace sbash64::budget

#endif
