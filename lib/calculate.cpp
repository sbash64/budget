#include "calculate.hpp"

namespace sbash64 {
namespace budget {
namespace calculate {
auto difference(Income income, Expenses) -> USD { return income.usd; }
} // namespace calculate
} // namespace budget
} // namespace sbash64
