#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace sbash64::budget {
struct USD {
  std::int_least64_t cents;
};

inline auto operator+(USD a, USD b) -> USD { return USD{a.cents + b.cents}; }

inline auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

struct Income {
  USD usd;
};

struct ExpenseCategory {
  std::string name;
};

inline auto operator<(const ExpenseCategory &a, const ExpenseCategory &b)
    -> bool {
  return a.name < b.name;
}

inline auto operator==(const ExpenseCategory &a, const ExpenseCategory &b)
    -> bool {
  return a.name == b.name;
}

struct Expense {
  USD usd{};
  ExpenseCategory category{};
};

struct Expenses {
  std::vector<Expense> all;
};

struct ExpenseTree {
  std::map<ExpenseCategory, std::variant<ExpenseTree, USD>>
      categorizedExpenseTreesOrCosts;
};

template <typename T> class rvalue_reference_wrapper {
public:
  rvalue_reference_wrapper<T>(const T &rvalue) : rvalue{rvalue} {}

  operator T() const { return rvalue; }

private:
  const T &rvalue;
};

struct RecursiveCategory;

using Subcategory = rvalue_reference_wrapper<RecursiveCategory>;

struct RecursiveCategory {
  ExpenseCategory category;
  std::optional<Subcategory> maybeSubcategory{};
};

struct RecursiveExpense;

using Subexpense = rvalue_reference_wrapper<RecursiveExpense>;

struct RecursiveExpense {
  ExpenseCategory category;
  std::variant<USD, Subexpense> subexpenseOrUsd;
};
} // namespace sbash64::budget

#endif
