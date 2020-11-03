#include "print.hpp"
#include <cstdio>
#include <string>

namespace sbash64 {
namespace budget {
namespace print {
static auto format_(USD usd) -> std::string {
  char cents[3];
  std::snprintf(cents, sizeof cents, "%.2lld", usd.cents % 100);
  return '$' + std::to_string(usd.cents / 100) + '.' + std::string{cents};
}

auto format(USD usd) -> std::string { return format_(usd); }

void pretty(std::ostream &stream, Income income, const Expenses &) {
  stream << "Income: " << format_(income.usd) << "\n";
  stream << "Expenses: $0.00\n";
  stream << "Difference: " << format_(income.usd);
}
} // namespace print
} // namespace budget
} // namespace sbash64