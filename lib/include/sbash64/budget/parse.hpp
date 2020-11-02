#ifndef SBASH64_BUDGET_PARSE_HPP_
#define SBASH64_BUDGET_PARSE_HPP_

#include <cstdint>
#include <string_view>

namespace sbash64 {
namespace budget {
struct USD {
  std::int_least64_t cents;
};
namespace parse {
auto usd(std::string_view) -> USD;
} // namespace parse
} // namespace budget
} // namespace sbash64

#endif
