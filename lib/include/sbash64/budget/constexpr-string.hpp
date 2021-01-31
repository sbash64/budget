#ifndef SBASH64_BUDGET_CONSTEXPR_STRING_HPP_
#define SBASH64_BUDGET_CONSTEXPR_STRING_HPP_

#include <cstddef>

// https://stackoverflow.com/a/65440575

// we cannot return a char array from a function, therefore we need a wrapper
template <unsigned N> struct String { char c[N]; };

template <unsigned... Len>
constexpr auto concatenate(const char (&...strings)[Len]) {
  constexpr auto N{(... + Len) - sizeof...(Len)};
  String<N + 1> result = {};
  result.c[N] = '\0';

  auto *dst{result.c};
  for (const auto *src : {strings...})
    for (; *src != '\0'; src++, dst++)
      *dst = *src;
  return result;
}

// https://stackoverflow.com/a/26082447
template <size_t N> constexpr auto length(char const (&)[N]) -> size_t {
  return N - 1;
}

#endif
