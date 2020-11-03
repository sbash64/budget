#include "calculate.hpp"

namespace sbash64 {
namespace budget {
namespace calculate {
constexpr auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

auto difference(Income income, Expenses expenses) -> USD {
  return income.usd -
         (expenses.all.empty() ? USD{0} : expenses.all.front().usd);
}
} // namespace calculate
} // namespace budget
} // namespace sbash64
