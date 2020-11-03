#include "calculate.hpp"
#include <numeric>

namespace sbash64 {
namespace budget {
namespace calculate {
constexpr auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

constexpr auto operator+(USD a, USD b) -> USD { return USD{a.cents + b.cents}; }

static auto operator==(const Category &a, const Category &b) -> bool {
  return a.name == b.name;
}

static auto total(const Expenses &expenses) -> USD {
  return std::accumulate(expenses.all.begin(), expenses.all.end(), USD{0},
                         [](USD usd, Expense a) -> USD { return a.usd + usd; });
}

auto difference(Income income, const Expenses &expenses) -> USD {
  return income.usd - total(expenses);
}

auto total(Category category, const Expenses &expenses) -> USD {
  return std::accumulate(expenses.all.begin(), expenses.all.end(), USD{0},
                         [=](USD usd, Expense a) -> USD {
                           return category == a.category ? a.usd + usd : usd;
                         });
}
} // namespace calculate
} // namespace budget
} // namespace sbash64
