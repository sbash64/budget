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
  assertEqual(result, gsl::narrow<unsigned long long>(expected.cents),
              gsl::narrow<unsigned long long>(actual.cents));
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
} // namespace sbash64::budget
