#include "calculate.hpp"
#include <numeric>

namespace sbash64 {
namespace budget {
namespace calculate {
constexpr auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

constexpr auto operator+(USD a, USD b) -> USD { return USD{a.cents + b.cents}; }

auto total(Expenses expenses) -> USD {
  return std::accumulate(expenses.all.begin(), expenses.all.end(), USD{0},
                         [](USD usd, Expense a) { return a.usd + usd; });
}

auto difference(Income income, Expenses expenses) -> USD {
  return income.usd - total(expenses);
}
} // namespace calculate
} // namespace budget
} // namespace sbash64
