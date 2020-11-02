#include "parse.hpp"
#include <iomanip>
#include <sstream>

namespace sbash64 {
namespace budget {
namespace parse {
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
} // namespace parse
} // namespace budget
} // namespace sbash64