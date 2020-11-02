#include "parse.hpp"
#include <iomanip>
#include <sstream>

namespace sbash64 {
namespace budget {
namespace parse {
auto usd(std::string_view s) -> USD {
  USD usd{};
  const auto decimalPoint{s.find('.')};
  if (decimalPoint == std::string::npos) {
    std::stringstream stream{s.data()};
    stream >> usd.cents;
    usd.cents *= 100;
  } else {
    {
      std::stringstream stream{s.substr(0, decimalPoint).data()};
      stream >> usd.cents;
      usd.cents *= 100;
    }
    {
      std::stringstream stream{s.substr(decimalPoint + 1).data()};
      int cents;
      stream >> cents;
      usd.cents += cents;
    }
  }
  return usd;
}
} // namespace parse
} // namespace budget
} // namespace sbash64