#ifndef SBASH64_BUDGET_TRANSACTION_HPP_
#define SBASH64_BUDGET_TRANSACTION_HPP_

#include <cstdint>
#include <string>

namespace sbash64::budget {
struct USD {
  std::int_least64_t cents;

  auto operator==(const USD &) const -> bool = default;
};

constexpr auto operator+(USD a, USD b) -> USD { return USD{a.cents + b.cents}; }

constexpr auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

constexpr auto operator-(USD a) -> USD { return USD{-a.cents}; }

enum class Month {
  January = 1,
  February,
  March,
  April,
  May,
  June,
  July,
  August,
  September,
  October,
  November,
  December
};

struct Date {
  int year;
  Month month;
  int day;

  auto operator==(const Date &) const -> bool = default;
};

constexpr auto operator<(const Date &a, const Date &b) -> bool {
  if (a.year != b.year)
    return a.year < b.year;
  if (a.month != b.month)
    return a.month < b.month;
  return a.day < b.day;
}

struct Transaction {
  USD amount;
  std::string description;
  Date date;

  auto operator==(const Transaction &) const -> bool = default;
};

static auto operator<(const Transaction &a, const Transaction &b) -> bool {
  if (a.date != b.date)
    return a.date < b.date;
  if (a.description != b.description)
    return a.description < b.description;
  if (a.amount != b.amount)
    return a.amount.cents < b.amount.cents;
  return false;
}
} // namespace sbash64::budget

#endif
