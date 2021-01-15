#include "command-line.hpp"
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace sbash64::budget {
enum class CommandLineInterpreter::State {
  normal,
  readyForAccountName,
  readyForAmount,
  readyForDate,
  readyForDescription,
  readyForNewName
};

enum class CommandLineInterpreter::CommandType {
  transaction,
  transfer,
  rename
};

void execute(CommandLineInterpreter &controller, Model &model,
             CommandLineInterface &interface,
             SessionSerialization &serialization,
             SessionDeserialization &deserialization, std::string_view input) {
  controller.execute(model, interface, serialization, deserialization, input);
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

static auto description(std::string_view s) -> std::string {
  return std::string{s};
}

static auto transaction(USD amount, const Date &date, std::string_view input)
    -> Transaction {
  return Transaction{amount, description(input), date};
}

static void enterTransaction(Model &model, CommandLineInterface &interface,
                             CommandLineInterpreter::State &state,
                             Transaction::Type transactionType, USD amount,
                             std::string_view accountName, const Date &date,
                             std::string_view input) {
  switch (transactionType) {
  case Transaction::Type::credit:
    model.credit(transaction(amount, date, input));
    break;
  case Transaction::Type::debit:
    model.debit(accountName, transaction(amount, date, input));
    break;
  }
  interface.show(transaction(amount, date, input),
                 "-> " + std::string{accountName});
  state = CommandLineInterpreter::State::normal;
}

static void parseDate(Model &model, CommandLineInterface &interface,
                      CommandLineInterpreter::State &state, Date &date,
                      USD amount, std::string_view accountName,
                      CommandLineInterpreter::CommandType commandType,
                      std::string_view input) {
  switch (commandType) {
  case CommandLineInterpreter::CommandType::transaction:
    date = budget::date(input);
    state = CommandLineInterpreter::State::readyForDescription;
    interface.prompt("description [anything]");
    break;
  case CommandLineInterpreter::CommandType::rename:
    break;
  case CommandLineInterpreter::CommandType::transfer:
    model.transferTo(accountName, amount, budget::date(input));
    state = CommandLineInterpreter::State::normal;
  }
}

static void executeFirstLineOfMultiLineCommand(
    CommandLineInterface &interface, CommandLineInterpreter::State &state,
    CommandLineInterpreter::CommandType &commandType,
    Transaction::Type &transactionType, std::string_view commandName) {
  if (commandName == "credit") {
    transactionType = Transaction::Type::credit;
    commandType = CommandLineInterpreter::CommandType::transaction;
    state = CommandLineInterpreter::State::readyForAmount;
    interface.prompt("how much? [amount ($)]");
  } else {
    if (commandName == "rename") {
      commandType = CommandLineInterpreter::CommandType::rename;
    } else if (commandName == "debit") {
      transactionType = Transaction::Type::debit;
      commandType = CommandLineInterpreter::CommandType::transaction;
    } else if (commandName == "transferto") {
      commandType = CommandLineInterpreter::CommandType::transfer;
    }
    state = CommandLineInterpreter::State::readyForAccountName;
    interface.prompt("which account? [name]");
  }
}

static void executeCommand(Model &model, CommandLineInterface &interface,
                           SessionSerialization &serialization,
                           SessionDeserialization &deserialization,
                           CommandLineInterpreter::State &state,
                           CommandLineInterpreter::CommandType &commandType,
                           Transaction::Type &transactionType,
                           std::string_view input) {
  std::stringstream stream{std::string{input}};
  std::string commandName;
  stream >> commandName;
  if (commandName == "print")
    model.show(interface);
  else if (commandName == "save")
    model.save(serialization);
  else if (commandName == "load")
    model.load(deserialization);
  else if (commandName == "credit" || commandName == "debit" ||
           commandName == "rename" || commandName == "transferto")
    executeFirstLineOfMultiLineCommand(interface, state, commandType,
                                       transactionType, commandName);
  else
    interface.show("unknown command \"" + commandName + '"');
}

CommandLineInterpreter::CommandLineInterpreter()
    : state{State::normal}, commandType{CommandType::transaction},
      transactionType{Transaction::Type::credit} {}

void CommandLineInterpreter::execute(Model &model,
                                     CommandLineInterface &interface,
                                     SessionSerialization &serialization,
                                     SessionDeserialization &deserialization,
                                     std::string_view input) {
  switch (state) {
  case State::normal:
    return executeCommand(model, interface, serialization, deserialization,
                          state, commandType, transactionType, input);
  case State::readyForAccountName:
    accountName = input;
    if (commandType == CommandType::rename) {
      state = CommandLineInterpreter::State::readyForNewName;
      interface.prompt("new name [anything]");
    } else {
      state = State::readyForAmount;
      interface.prompt("how much? [amount ($)]");
    }
    break;
  case State::readyForAmount:
    amount = usd(input);
    state = State::readyForDate;
    interface.prompt("date [month day year]");
    break;
  case State::readyForDate:
    return parseDate(model, interface, state, date, amount, accountName,
                     commandType, input);
  case State::readyForDescription:
    return enterTransaction(model, interface, state, transactionType, amount,
                            accountName, date, input);
  case State::readyForNewName:
    model.renameAccount(accountName, input);
    state = State::normal;
  }
}

static auto prepareLengthTwoInteger(std::ostream &stream) -> std::ostream & {
  return stream << std::setw(2) << std::setfill('0');
}

static auto operator<<(std::ostream &stream, USD usd) -> std::ostream & {
  const auto fill{stream.fill()};
  return prepareLengthTwoInteger(stream << usd.cents / 100 << '.')
         << std::abs(usd.cents % 100) << std::setfill(fill);
}

static auto formatWithoutDollarSign(USD usd) -> std::string {
  std::stringstream stream;
  stream << usd;
  return stream.str();
}

static auto putWithDollarSign(std::ostream &stream, USD usd) -> std::ostream & {
  return stream << '$' << usd;
}

auto format(USD usd) -> std::string {
  std::stringstream stream;
  putWithDollarSign(stream, usd);
  return stream.str();
}

constexpr auto to_integral(Month e) ->
    typename std::underlying_type<Month>::type {
  return static_cast<typename std::underlying_type<Month>::type>(e);
}

static auto operator<<(std::ostream &stream, const Month &month)
    -> std::ostream & {
  return stream << to_integral(month);
}

static auto operator<<(std::ostream &stream, const Date &date)
    -> std::ostream & {
  const auto fill{stream.fill()};
  return prepareLengthTwoInteger(prepareLengthTwoInteger(stream)
                                 << date.month << '/')
         << date.day << '/' << date.year << std::setfill(fill);
}

CommandLineStream::CommandLineStream(std::ostream &stream) : stream{stream} {}

static auto putNewLine(std::ostream &stream) -> std::ostream & {
  return stream << '\n';
}

void CommandLineStream::show(Account &primary,
                             const std::vector<Account *> &secondaries) {
  primary.show(*this);
  for (auto *account : secondaries) {
    putNewLine(putNewLine(stream));
    account->show(*this);
  }
  putNewLine(stream);
}

static auto putSpaces(std::ostream &stream, int n) -> std::ostream & {
  return stream << std::setw(n) << "";
}

void CommandLineStream::showAccountSummary(
    std::string_view name, USD balance,
    const std::vector<TransactionWithType> &transactions) {
  putNewLine(stream << "----");
  putNewLine(stream << name);
  putNewLine(putWithDollarSign(stream, balance));
  putNewLine(stream);
  stream << "Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description";
  for (const auto &transaction : transactions) {
    putNewLine(stream);
    auto width{25};
    if (transaction.type == Transaction::Type::credit) {
      const auto spaces{12};
      putSpaces(stream, spaces);
      width -= spaces;
    }
    putSpaces(stream << std::setw(width) << std::left
                     << formatWithoutDollarSign(transaction.transaction.amount)
                     << std::right << transaction.transaction.date,
              10)
        << transaction.transaction.description;
  }
  putNewLine(stream) << "----";
}

static auto putSpace(std::ostream &stream) -> std::ostream & {
  return stream << ' ';
}

void CommandLineStream::prompt(std::string_view s) { putSpace(stream << s); }

void CommandLineStream::show(const Transaction &t, std::string_view suffix) {
  putNewLine(putSpace(putSpace(putSpace(putWithDollarSign(stream, t.amount))
                               << t.date.month << '/' << t.date.day << '/'
                               << t.date.year)
                      << t.description)
             << suffix);
}

void CommandLineStream::show(std::string_view message) {
  putNewLine(stream << message);
}

auto usd(std::string_view s) -> USD {
  USD usd{};
  std::istringstream stream{std::string{s}};
  if (stream.peek() != '.') {
    stream >> usd.cents;
    usd.cents *= 100;
  }
  if (stream.get() == '.') {
    std::string afterDecimal;
    stream >> afterDecimal;
    afterDecimal.resize(2, '0');
    std::istringstream streamAfterDecimal{afterDecimal};
    int cents = 0;
    streamAfterDecimal >> cents;
    usd.cents += cents;
  }
  return usd;
}
} // namespace sbash64::budget