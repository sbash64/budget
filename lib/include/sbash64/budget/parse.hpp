#ifndef SBASH64_BUDGET_PARSE_HPP_
#define SBASH64_BUDGET_PARSE_HPP_

#include "domain.hpp"

#include <string_view>

namespace sbash64::budget {
auto usd(std::string_view) -> USD;
} // namespace sbash64::budget

#endif
