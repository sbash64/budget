#include "usd.hpp"
#include <gsl/gsl>

namespace sbash64::budget {
auto operator"" _cents(unsigned long long cents) -> USD {
  return USD{gsl::narrow<int_least64_t>(cents)};
}

constexpr auto to_integral(Month e) ->
    typename std::underlying_type<Month>::type {
  return static_cast<typename std::underlying_type<Month>::type>(e);
}

void assertEqual(testcpplite::TestResult &result, USD expected, USD actual) {
  assertEqual(result, expected.cents, actual.cents);
}

void assertEqual(testcpplite::TestResult &result, const Date &expected,
                 const Date &actual) {
  assertEqual(result, expected.day, actual.day);
  assertEqual(result, to_integral(expected.month), to_integral(actual.month));
  assertEqual(result, expected.year, actual.year);
}

void assertEqual(testcpplite::TestResult &result, const Transaction &expected,
                 const Transaction &actual) {
  assertEqual(result, expected.amount, actual.amount);
  assertEqual(result, expected.description, actual.description);
  assertEqual(result, expected.date, actual.date);
}

void assertEqual(testcpplite::TestResult &result,
                 const VerifiableTransaction &expected,
                 const VerifiableTransaction &actual) {
  assertEqual(result, expected.transaction, actual.transaction);
  assertEqual(result, static_cast<int>(expected.verified),
              static_cast<int>(actual.verified));
}

void assertEqual(testcpplite::TestResult &result, const Transactions &expected,
                 const Transactions &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (Transactions::size_type i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}
} // namespace sbash64::budget
