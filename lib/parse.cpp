#include "parse.hpp"
#include <iomanip>
#include <sstream>

namespace sbash64 {
namespace budget {
namespace parse {
auto usd(std::string_view s) -> USD {
  USD usd{};
  std::stringstream stream{s.data()};
  stream >> usd.cents;
  return usd;
}
} // namespace parse
} // namespace budget
} // namespace sbash64