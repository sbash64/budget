#ifndef SBASH64_BUDGET_TEST_USD_HPP_
#define SBASH64_BUDGET_TEST_USD_HPP_

#include <sbash64/budget/budget.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
auto operator"" _cents(unsigned long long cents) -> USD;

void assertEqual(testcpplite::TestResult &result, USD expected, USD actual);
} // namespace sbash64::budget

#endif
