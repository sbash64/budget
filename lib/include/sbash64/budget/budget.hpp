#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include <cstdint>

namespace sbash64 {
namespace budget {
struct USD {
  std::int_least64_t cents;
};
} // namespace budget
} // namespace sbash64

#endif
