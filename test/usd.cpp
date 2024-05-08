#include "usd.hpp"

#include <gsl/gsl>

namespace sbash64::budget {
auto operator"" _cents(unsigned long long cents) -> USD {
  return USD{gsl::narrow<int_least64_t>(cents)};
}

constexpr auto to_integral(Month e) -> std::underlying_type_t<Month> {
  return static_cast<std::underlying_type_t<Month>>(e);
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
                 const ArchivableVerifiableTransaction &expected,
                 const ArchivableVerifiableTransaction &actual) {
  assertEqual(result, static_cast<const Transaction &>(expected),
              static_cast<const Transaction &>(actual));
  assertEqual(result, static_cast<int>(expected.verified),
              static_cast<int>(actual.verified));
  assertEqual(result, static_cast<int>(expected.archived),
              static_cast<int>(actual.archived));
}

void assertEqual(testcpplite::TestResult &result,
                 const std::vector<Transaction> &expected,
                 const std::vector<Transaction> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (std::vector<Transaction>::size_type i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}
} // namespace sbash64::budget
