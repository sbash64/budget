#include "parse.hpp"
#include <iomanip>
#include <sstream>

namespace sbash64 {
namespace budget {
namespace parse {
auto usd(std::string_view s) -> USD {
  USD usd{};
  std::istringstream stream{s.data()};
  stream >> usd.cents;
  usd.cents *= 100;
  char possiblyDecimal{};
  stream >> possiblyDecimal;
  if (possiblyDecimal == '.') {
    int cents = 0;
    stream >> cents;
    usd.cents += cents;
  }
  return usd;
}
} // namespace parse
} // namespace budget
} // namespace sbash64