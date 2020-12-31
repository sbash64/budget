#include "parse.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace sbash64::budget::parse {
auto usd(std::string_view s) -> USD {
  USD usd{};
  std::istringstream stream{s.data()};
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

auto isUsd(std::string_view s) -> bool {
  return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) {
    return (std::isdigit(c) != 0) || c == '.';
  });
}
} // namespace sbash64::budget::parse