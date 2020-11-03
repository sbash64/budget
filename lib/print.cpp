#include "print.hpp"

namespace sbash64 {
namespace budget {
namespace print {
void pretty(std::ostream &stream, Income, const Expenses &) {
  stream << "Income: $0.00\n";
  stream << "Expenses: $0.00\n";
  stream << "Difference: $0.00";
}

auto format(USD usd) -> std::string {
  return "$" + std::to_string(usd.cents / 100) + ".0" +
         std::to_string(usd.cents % 100);
}
} // namespace print
} // namespace budget
} // namespace sbash64