#include "evaluate.hpp"
#include "parse.hpp"
#include <sstream>
#include <vector>

namespace sbash64::budget::evaluate {
static void initialize(RecursiveExpense &expense, std::stringstream &stream,
                       std::string_view category,
                       std::vector<RecursiveExpense> &expenses) {
  expense.category.name = category;
  std::string next;
  stream >> next;
  if (parse::isUsd(next)) {
    expense.subexpenseOrUsd = parse::usd(next);
  } else {
    expenses.push_back({});
    auto &subexpense{expenses.back()};
    initialize(subexpense, stream, next, expenses);
    expense.subexpenseOrUsd.emplace<Subexpense>(subexpense);
  }
}

void command(ExpenseRecord &record, std::string_view s) {
  LabeledExpense expense;
  std::stringstream stream{s.data()};
  std::string category;
  stream >> category;
  std::vector<RecursiveExpense> expenses;
  initialize(expense.expense, stream, category, expenses);
  stream >> std::ws;
  std::string label;
  getline(stream, label);
  expense.label = label;
  record.enter(expense);
}
} // namespace sbash64::budget::evaluate