#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>

namespace sbash64::budget {
struct USD {
  std::int_least64_t cents;
};

inline auto operator+(USD a, USD b) -> USD { return USD{a.cents + b.cents}; }

inline auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

inline auto operator==(USD a, USD b) -> bool { return a.cents == b.cents; }

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

struct ExpenseTree {
  std::map<ExpenseCategory, std::variant<ExpenseTree, USD>>
      categorizedExpenseTreesOrTotalUsds;
};

inline auto operator==(const ExpenseTree &a, const ExpenseTree &b) -> bool {
  return a.categorizedExpenseTreesOrTotalUsds ==
         b.categorizedExpenseTreesOrTotalUsds;
}

template <typename T> class rvalue_reference_wrapper {
public:
  rvalue_reference_wrapper<T>(const T &rvalue) : rvalue{rvalue} {}
  operator T() const { return rvalue; }

private:
  const T &rvalue;
};

struct RecursiveExpenseCategory;

using ExpenseSubcategory = rvalue_reference_wrapper<RecursiveExpenseCategory>;

struct RecursiveExpenseCategory {
  ExpenseCategory category;
  std::optional<ExpenseSubcategory> maybeSubcategory{};
};

struct RecursiveExpense;

using Subexpense = rvalue_reference_wrapper<RecursiveExpense>;

struct RecursiveExpense {
  ExpenseCategory category;
  std::variant<USD, Subexpense> subexpenseOrUsd;
};

inline auto operator==(const RecursiveExpense &a, const RecursiveExpense &b)
    -> bool {
  return a.subexpenseOrUsd == b.subexpenseOrUsd && a.category == b.category;
}

struct LabeledExpense {
  RecursiveExpense expense;
  std::string label;
};

inline auto operator==(const LabeledExpense &a, const LabeledExpense &b)
    -> bool {
  return a.expense == b.expense && a.label == b.label;
}

#define SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(a)                   \
  virtual ~a() = default;                                                      \
  a() = default;                                                               \
  a(const a &) = delete;                                                       \
  a(a &&) = delete;                                                            \
  auto operator=(const a &)->a & = delete;                                     \
  auto operator=(a &&)->a & = delete

class ExpenseRecord {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(ExpenseRecord);
  virtual void enter(const LabeledExpense &) = 0;
};
} // namespace sbash64::budget

#endif
