#include "evaluate.hpp"
#include "parse.hpp"
#include <forward_list>
#include <sstream>

namespace sbash64::budget::evaluate {
static auto getNext(std::stringstream &stream) -> std::string {
  stream >> std::ws;
  std::string next;
  if (stream.peek() == '"') {
    stream.get();
    getline(stream, next, '"');
    stream.get();
  } else {
    stream >> next;
  }
  return next;
}

static void initialize(RecursiveExpense &expense, std::stringstream &stream,
                       std::string_view category,
                       std::forward_list<RecursiveExpense> &expenses) {
  expense.category.name = category;
  auto next{getNext(stream)};
  if (parse::isUsd(next)) {
    expense.subexpenseOrUsd = parse::usd(next);
  } else {
    expenses.push_front({});
    auto &subexpense{expenses.front()};
    initialize(subexpense, stream, next, expenses);
    expense.subexpenseOrUsd.emplace<Subexpense>(subexpense);
  }
}

void command(ExpenseRecord &record, std::string_view s) {
  LabeledExpense expense;
  std::stringstream stream{s.data()};
  std::string category;
  stream >> category;
  std::forward_list<RecursiveExpense> expenses;
  initialize(expense.expense, stream, category, expenses);
  stream >> std::ws;
  std::string label;
  getline(stream, label);
  expense.label = label;
  record.enter(expense);
}
} // namespace sbash64::budget::evaluate