#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
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

struct Category {
  std::string name;
};

inline auto operator<(const Category &a, const Category &b) -> bool {
  return a.name < b.name;
}

inline auto operator==(const Category &a, const Category &b) -> bool {
  return a.name == b.name;
}

struct ExpenseTree {
  std::map<Category, std::variant<ExpenseTree, USD>> expenseTreeOrUsd;
};

inline auto operator==(const ExpenseTree &a, const ExpenseTree &b) -> bool {
  return a.expenseTreeOrUsd == b.expenseTreeOrUsd;
}

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
  Category category;
  std::optional<Subcategory> maybeSubcategory{};
};

struct RecursiveExpense;

using Subexpense = rvalue_reference_wrapper<RecursiveExpense>;

struct RecursiveExpense {
  Category category;
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
  virtual void print(std::ostream &) = 0;
};

enum class Month {
  January = 1,
  February,
  March,
  April,
  May,
  June,
  July,
  August,
  September,
  October,
  November,
  December
};

struct Date {
  int year;
  Month month;
  int day;
};

struct Transaction {
  USD amount;
  std::string description;
  Date date;
};

struct PrintableTransaction {
  Transaction transaction;
  bool debit{};
};

inline auto printableDebit(Transaction transaction) -> PrintableTransaction {
  return {std::move(transaction), true};
}

inline auto printableCredit(Transaction transaction) -> PrintableTransaction {
  return {std::move(transaction), false};
}

class Bank {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Bank);
  virtual void debit(std::string_view accountName, const Transaction &) = 0;
  virtual void credit(const Transaction &) = 0;
};
} // namespace sbash64::budget

#endif
