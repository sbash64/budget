#include "format.hpp"

#include <iomanip>

namespace sbash64::budget {
constexpr auto to_integral(Month e) ->
    typename std::underlying_type<Month>::type {
  return static_cast<typename std::underlying_type<Month>::type>(e);
}

static auto prepareLengthTwoInteger(std::ostream &stream) -> std::ostream & {
  return stream << std::setw(2) << std::setfill('0');
}

auto operator<<(std::ostream &stream, USD usd) -> std::ostream & {
  const auto fill{stream.fill()};
  if (usd.cents < 0)
    stream << '-';
  return prepareLengthTwoInteger(stream << std::abs(usd.cents / 100) << '.')
         << std::abs(usd.cents % 100) << std::setfill(fill);
}

auto operator<<(std::ostream &stream, const Month &month) -> std::ostream & {
  return stream << to_integral(month);
}

auto operator<<(std::ostream &stream, const Date &date) -> std::ostream & {
  const auto fill{stream.fill()};
  return prepareLengthTwoInteger(prepareLengthTwoInteger(stream)
                                 << date.month << '/')
         << date.day << '/' << date.year << std::setfill(fill);
}

auto putWithDollarSign(std::ostream &stream, USD usd) -> std::ostream & {
  return stream << '$' << usd;
}
} // namespace sbash64::budget
