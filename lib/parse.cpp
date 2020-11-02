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
    std::string remaining;
    stream >> remaining;
    remaining.resize(2, '0');
    std::istringstream last{remaining};
    int cents = 0;
    last >> cents;
    usd.cents += cents;
  }
  return usd;
}
} // namespace parse
} // namespace budget
} // namespace sbash64