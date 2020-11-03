#include "print.hpp"
#include <cstdio>
#include <string>

namespace sbash64 {
namespace budget {
namespace print {
void pretty(std::ostream &stream, Income, const Expenses &) {
  stream << "Income: $0.00\n";
  stream << "Expenses: $0.00\n";
  stream << "Difference: $0.00";
}

auto format(USD usd) -> std::string {
  char cents[3];
  std::snprintf(cents, sizeof cents, "%.2lld", usd.cents % 100);
  return "$" + std::to_string(usd.cents / 100) + "." + std::string{cents};
}
} // namespace print
} // namespace budget
} // namespace sbash64