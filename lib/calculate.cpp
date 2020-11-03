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

static auto total_(const Expenses &expenses) -> USD {
  return std::accumulate(
      expenses.all.begin(), expenses.all.end(), USD{0},
      [](USD usd, const Expense &expense) -> USD { return expense.usd + usd; });
}

auto total(const Expenses &expenses) -> USD { return total_(expenses); }

auto difference(Income income, const Expenses &expenses) -> USD {
  return income.usd - total_(expenses);
}

auto total(const Category &category, const Expenses &expenses) -> USD {
  return std::accumulate(expenses.all.begin(), expenses.all.end(), USD{0},
                         [=](USD usd, const Expense &expense) -> USD {
                           return category == expense.category
                                      ? expense.usd + usd
                                      : usd;
                         });
}

auto categories(const Expenses &expenses) -> Categories {
  Categories categories;
  std::for_each(expenses.all.begin(), expenses.all.end(),
                [&](const Expense &expense) {
                  categories.each.push_back(expense.category);
                });
  return categories;
}
} // namespace calculate
} // namespace budget
} // namespace sbash64
