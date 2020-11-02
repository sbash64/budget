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
  char possiblyDecimal{};
  stream >> possiblyDecimal;
  if (possiblyDecimal == '.') {
    int cents = 0;
    auto peek{stream.peek()};
    stream >> cents;
    if (peek == '0')
      while (cents > 10)
        cents /= 10;
    else if (cents < 10)
      cents *= 10;
    while (cents > 100)
      cents /= 10;
    usd.cents += cents;
  }
  return usd;
}
} // namespace parse
} // namespace budget
} // namespace sbash64