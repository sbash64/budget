#include "command-line.hpp"
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

void command(CommandLineInterpreter &controller, Model &model,
             CommandLineInterface &view, SessionSerialization &serialization,
             SessionDeserialization &deserialization, std::string_view input) {
  controller.command(model, view, serialization, deserialization, input);
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

void CommandLineInterpreter::command(Model &bank, CommandLineInterface &view,
                                     SessionSerialization &serialization,
                                     SessionDeserialization &deserialization,
                                     std::string_view input) {
  std::stringstream stream{std::string{input}};
  std::string commandName;
  switch (state) {
  case State::normal:
    stream >> commandName;
    if (commandName == "print") {
      bank.show(view);
    } else if (commandName == "save") {
      bank.save(serialization);
    } else if (commandName == "load") {
      bank.load(deserialization);
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
      view.prompt("date [month day year]");
    }
    break;
  case State::readyForDate:
    date = budget::date(input);
    switch (commandType) {
    case CommandType::transaction:
      state = State::readyForDescription;
      view.prompt("description [anything]");
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
    view.show(Transaction{amount, std::string{input}, date},
              "-> " + accountName);
    state = State::normal;
    break;
  }
}
} // namespace sbash64::budget