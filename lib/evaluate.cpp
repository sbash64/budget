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

class NoAmountFound {};

static void initialize(RecursiveExpense &expense, std::stringstream &stream,
                       std::string_view category,
                       std::forward_list<RecursiveExpense> &expenses) {
  expense.category.name = category;
  const auto argument{nextArgument(stream)};
  if (argument.empty())
    throw NoAmountFound{};
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
    } catch (const NoAmountFound &) {
      output << "No expense entered because no amount found.";
    }
  }
}

void command(Controller &c, Bank &b, std::string_view s) { c.command(b, s); }

static auto next(std::stringstream &stream) -> std::string {
  std::string next;
  stream >> next;
  return next;
}

static auto date(std::string_view s) -> Date {
  Date date;
  std::stringstream stream{s.data()};
  int month;
  stream >> month;
  int day;
  stream >> day;
  int year;
  stream >> year;
  date.month = Month{month};
  date.day = day;
  date.year = year + 2000;
  return date;
}

void Controller::command(Bank &bank, std::string_view input) {
  switch (state) {
  case State::normal:
    break;
  case State::readyForDebitDate:
    debitDate = date(input);
    state = State::readyForDebitDescription;
    return;
  case State::readyForDebitDescription:
    bank.debit(debitAccountName,
               Transaction{debitAmount, input.data(), debitDate});
    state = State::normal;
    return;
  }
  std::stringstream stream{input.data()};
  std::string commandName;
  stream >> commandName;
  debitAccountName = next(stream);
  debitAmount = parse::usd(next(stream));
  state = State::readyForDebitDate;
}
} // namespace sbash64::budget::evaluate