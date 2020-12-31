#include "evaluate.hpp"
#include "parse.hpp"
#include "print.hpp"
#include <forward_list>
#include <sstream>

namespace sbash64::budget::evaluate {
constexpr auto multiWordArgumentDelimiter{'"'};

static auto nextArgument(std::stringstream &stream) -> std::string {
  stream >> std::ws;
  std::string argument;
  if (stream.peek() == multiWordArgumentDelimiter) {
    stream.get();
    getline(stream, argument, multiWordArgumentDelimiter);
    stream.get();
  } else {
    stream >> argument;
  }
  return argument;
}

class InvalidCommand {};

static void initialize(RecursiveExpense &expense, std::stringstream &stream,
                       std::string_view category,
                       std::forward_list<RecursiveExpense> &expenses) {
  expense.category.name = category;
  const auto argument{nextArgument(stream)};
  if (argument.empty())
    throw InvalidCommand{};
  if (parse::isUsd(argument)) {
    expense.subexpenseOrUsd = parse::usd(argument);
  } else {
    expenses.push_front({});
    auto &subexpense{expenses.front()};
    initialize(subexpense, stream, argument, expenses);
    expense.subexpenseOrUsd.emplace<Subexpense>(subexpense);
  }
}

void command(ExpenseRecord &record, std::string_view s, std::ostream &output) {
  LabeledExpense expense;
  std::stringstream stream{s.data()};
  std::string category;
  stream >> category;
  if (category == "print") {
    record.print(output);
  } else {
    std::forward_list<RecursiveExpense> expenses;
    try {
      initialize(expense.expense, stream, category, expenses);
      stream >> std::ws;
      std::string label;
      getline(stream, label);
      expense.label = label;
      record.enter(expense);
      print::pretty(output, expense);
    } catch (const InvalidCommand &) {
    }
  }
}
} // namespace sbash64::budget::evaluate