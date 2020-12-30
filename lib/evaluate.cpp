#include "evaluate.hpp"
#include "parse.hpp"
#include <sstream>
#include <vector>

namespace sbash64::budget::evaluate {
static auto isUsd(std::string_view s) -> bool { return s[0] == '2'; }

static void f(RecursiveExpense &expense, std::stringstream &stream,
              std::string_view category,
              std::vector<RecursiveExpense> &expenses) {
  expense.category.name = category;
  std::string next;
  stream >> next;
  if (isUsd(next)) {
    expense.subexpenseOrUsd = parse::usd(next);
  } else {
    expenses.push_back({});
    auto &subexpense{expenses.back()};
    f(subexpense, stream, next, expenses);
    expense.subexpenseOrUsd.emplace<Subexpense>(subexpense);
  }
}

void command(ExpenseRecord &record, std::string_view s) {
  std::stringstream stream{s.data()};
  std::string category;
  stream >> category;
  std::vector<RecursiveExpense> expenses;
  LabeledExpense expense;
  f(expense.expense, stream, category, expenses);
  stream.get();
  char b[100] = {'\0'};
  stream.getline(b, 100);
  std::string d{b};
  expense.label = d;
  record.enter(expense);
}
} // namespace sbash64::budget::evaluate