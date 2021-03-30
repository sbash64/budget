#ifndef SBASH64_BUDGET_CONSTEXPR_STRING_HPP_
#define SBASH64_BUDGET_CONSTEXPR_STRING_HPP_

#include <algorithm>
#include <array>
#include <cstddef>

template <std::size_t M, std::size_t N>
constexpr auto concatenate(const std::array<char, M> &a,
                           const std::array<char, N> &b) {
  std::array<char, M + N - 1> result{};
  for (std::size_t i{0}; i < a.size(); ++i)
    result[i] = a[i];
  for (std::size_t i{0}; i < b.size(); ++i)
    result[i + M - 1] = b[i];
  return result;
}

template <std::size_t M, std::size_t... N>
constexpr auto concatenate(const std::array<char, M> &a,
                           const std::array<char, N> &...b) {
  return concatenate(a, concatenate(b...));
}

template <std::size_t M>
constexpr auto length(const std::array<char, M> &) -> std::size_t {
  return M - 1;
}
#endif
