#ifndef SBASH64_BUDGET_PARSE_HPP_
#define SBASH64_BUDGET_PARSE_HPP_

#include "budget.hpp"
#include <string_view>

namespace sbash64::budget::parse {
auto usd(std::string_view) -> USD;
auto isUsd(std::string_view) -> bool;
} // namespace sbash64::budget::parse

#endif
