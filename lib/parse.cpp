#include "parse.hpp"
#include <sstream>
#include <string>

namespace sbash64::budget {
auto usd(std::string_view s) -> USD {
  USD usd{};
  std::istringstream stream{std::string{s}};
  if (stream.peek() != '.') {
    stream >> usd.cents;
    usd.cents *= 100;
  }
  if (stream.get() == '.') {
    std::string afterDecimal;
    stream >> afterDecimal;
    afterDecimal.resize(2, '0');
    std::istringstream streamAfterDecimal{afterDecimal};
    int cents = 0;
    streamAfterDecimal >> cents;
    usd.cents += cents;
  }
  return usd;
}
} // namespace sbash64::budget
