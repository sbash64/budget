#include "evaluate.hpp"
#include "parse.hpp"
#include <sstream>

namespace sbash64::budget {
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

void command(Controller &c, Model &b, View &p, std::string_view s) {
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

void Controller::command(Model &bank, View &printer, std::string_view input) {
  std::stringstream stream{std::string{input}};
  std::string commandName;
  switch (state) {
  case State::normal:
    stream >> commandName;
    if (commandName == "print") {
      bank.show(printer);
    } else {
      if (commandName == "transferto") {
        accountName = next(stream);
        commandType = CommandType::transfer;
      } else if (commandName == "debit") {
        accountName = next(stream);
        transactionType = Transaction::Type::debit;
        commandType = CommandType::transaction;
      } else if (commandName == "credit") {
        transactionType = Transaction::Type::credit;
        commandType = CommandType::transaction;
      }
      amount = usd(next(stream));
      state = State::readyForDate;
    }
    break;
  case State::readyForDate:
    date = budget::date(input);
    switch (commandType) {
    case CommandType::transaction:
      state = State::readyForDescription;
      break;
    case CommandType::transfer:
      bank.transferTo(accountName, amount, date);
      state = State::normal;
      break;
    }
    break;
  case State::readyForDescription:
    switch (transactionType) {
    case Transaction::Type::credit:
      bank.credit(Transaction{amount, std::string{input}, date});
      break;
    case Transaction::Type::debit:
      bank.debit(accountName, Transaction{amount, std::string{input}, date});
      break;
    }
    state = State::normal;
    break;
  }
}
} // namespace sbash64::budget