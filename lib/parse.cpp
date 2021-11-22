#include "parse.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace sbash64::budget {
auto usd(std::string_view s) -> USD {
  USD usd{};
  std::stringstream stream;
  stream << s;
  if (stream.peek() != '.' && stream.peek() != '-') {
    std::int_least64_t dollars = 0;
    stream >> dollars;
    usd.cents = dollars * 100;
  }
  if (stream.get() == '.' && stream.peek() != '-') {
    std::string afterDecimal;
    stream >> afterDecimal;
    afterDecimal.resize(2, '0');
    std::istringstream streamAfterDecimal{afterDecimal};
    std::int_least64_t cents = 0;
    streamAfterDecimal >> cents;
    usd.cents += cents;
  }
  return usd;
}
} // namespace sbash64::budget
