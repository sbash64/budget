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
  std::stringstream stream{std::string{s}};
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

void command(Controller &c, Model &b, Printer &p, std::string_view s) {
  c.command(b, p, s);
}

static auto next(std::stringstream &stream) -> std::string {
  std::string next;
  stream >> next;
  return next;
}

static auto date(std::string_view s) -> Date {
  Date date{};
  std::stringstream stream{std::string{s}};
  int month = 0;
  stream >> month;
  int day = 0;
  stream >> day;
  int year = 0;
  stream >> year;
  date.month = Month{month};
  date.day = day;
  date.year = year + 2000;
  return date;
}

void Controller::command(Model &bank, Printer &printer,
                         std::string_view input) {
  std::stringstream stream{std::string{input}};
  std::string commandName;
  switch (state) {
  case State::normal:
    stream >> commandName;
    if (commandName == "print") {
      bank.print(printer);
      return;
    }
    if (commandName == "debit") {
      debitAccountName = next(stream);
      transactionType = Transaction::Type::debit;
    } else {
      transactionType = Transaction::Type::credit;
    }
    amount = parse::usd(next(stream));
    state = State::readyForDate;
    return;
  case State::readyForDate:
    date = evaluate::date(input);
    state = State::readyForDescription;
    return;
  case State::readyForDescription:
    switch (transactionType) {
    case Transaction::Type::credit:
      bank.credit(Transaction{amount, std::string{input}, date});
      break;
    case Transaction::Type::debit:
      bank.debit(debitAccountName,
                 Transaction{amount, std::string{input}, date});
      break;
    }
    state = State::normal;
    return;
  }
}
} // namespace sbash64::budget::evaluate