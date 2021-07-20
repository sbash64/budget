#ifndef SBASH64_BUDGET_TEST_USD_HPP_
#define SBASH64_BUDGET_TEST_USD_HPP_

#include <sbash64/budget/domain.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
auto operator"" _cents(unsigned long long cents) -> USD;

void assertEqual(testcpplite::TestResult &result, USD expected, USD actual);

void assertEqual(testcpplite::TestResult &result, const Date &expected,
                 const Date &actual);

void assertEqual(testcpplite::TestResult &result, const Transaction &expected,
                 const Transaction &actual);

void assertEqual(testcpplite::TestResult &result,
                 const VerifiableTransaction &expected,
                 const VerifiableTransaction &actual);

void assertEqual(testcpplite::TestResult &result,
                 const std::vector<Transaction> &expected,
                 const std::vector<Transaction> &actual);
} // namespace sbash64::budget

#endif
