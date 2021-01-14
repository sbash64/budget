#include "command-line.hpp"
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace sbash64::budget {
enum class CommandLineInterpreter::State {
  normal,
  readyForDate,
  readyForDescription
};

enum class CommandLineInterpreter::CommandType { transaction, transfer };

void execute(CommandLineInterpreter &controller, Model &model,
             CommandLineInterface &interface,
             SessionSerialization &serialization,
             SessionDeserialization &deserialization, std::string_view input) {
  controller.command(model, interface, serialization, deserialization, input);
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

static void enterTransaction(CommandLineInterface &interface, Model &model,
                             CommandLineInterpreter::State &state,
                             Transaction::Type transactionType, USD amount,
                             std::string_view accountName, const Date &date,
                             std::string_view input) {
  switch (transactionType) {
  case Transaction::Type::credit:
    model.credit(Transaction{amount, std::string{input}, date});
    break;
  case Transaction::Type::debit:
    model.debit(accountName, Transaction{amount, std::string{input}, date});
    break;
  }
  interface.show(Transaction{amount, std::string{input}, date},
                 "-> " + std::string{accountName});
  state = CommandLineInterpreter::State::normal;
}

static void parseDate(CommandLineInterface &interface, Model &model,
                      CommandLineInterpreter::State &state, Date &date,
                      USD amount, std::string_view accountName,
                      CommandLineInterpreter::CommandType commandType,
                      std::string_view input) {
  date = budget::date(input);
  switch (commandType) {
  case CommandLineInterpreter::CommandType::transaction:
    state = CommandLineInterpreter::State::readyForDescription;
    interface.prompt("description [anything]");
    break;
  case CommandLineInterpreter::CommandType::transfer:
    model.transferTo(accountName, amount, date);
    state = CommandLineInterpreter::State::normal;
  }
}

CommandLineInterpreter::CommandLineInterpreter()
    : state{State::normal}, commandType{CommandType::transaction},
      transactionType{Transaction::Type::credit} {}

void CommandLineInterpreter::command(Model &model,
                                     CommandLineInterface &interface,
                                     SessionSerialization &serialization,
                                     SessionDeserialization &deserialization,
                                     std::string_view input) {
  std::stringstream stream{std::string{input}};
  std::string commandName;
  switch (state) {
  case State::normal:
    stream >> commandName;
    if (commandName == "print") {
      model.show(interface);
    } else if (commandName == "save") {
      model.save(serialization);
    } else if (commandName == "load") {
      model.load(deserialization);
    } else if (commandName == "rename") {
      std::string from;
      std::string next;
      stream >> next;
      auto first{true};
      while (next != "->") {
        if (!first)
          from += ' ';
        from += next;
        stream >> next;
        first = false;
      }
      stream >> std::ws;
      std::string to;
      getline(stream, to);
      model.renameAccount(from, to);
    } else {
      std::string eventuallyAmount;
      stream >> eventuallyAmount;
      if (commandName == "credit") {
        transactionType = Transaction::Type::credit;
        commandType = CommandType::transaction;
      } else {
        std::string accountName_;
        stream >> std::ws;
        auto first{true};
        while (!stream.eof()) {
          if (!first)
            accountName_ += ' ';
          accountName_ += eventuallyAmount;
          stream >> eventuallyAmount;
          stream >> std::ws;
          first = false;
        }
        accountName = accountName_;
        if (commandName == "transferto") {
          commandType = CommandType::transfer;
        } else if (commandName == "debit") {
          transactionType = Transaction::Type::debit;
          commandType = CommandType::transaction;
        }
      }
      amount = usd(eventuallyAmount);
      state = State::readyForDate;
      interface.prompt("date [month day year]");
    }
    break;
  case State::readyForDate:
    parseDate(interface, model, state, date, amount, accountName, commandType,
              input);
    break;
  case State::readyForDescription:
    enterTransaction(interface, model, state, transactionType, amount,
                     accountName, date, input);
    break;
  }
}

static auto operator<<(std::ostream &stream, USD usd) -> std::ostream & {
  const auto fill{stream.fill()};
  return stream << usd.cents / 100 << '.' << std::setw(2) << std::setfill('0')
                << std::abs(usd.cents % 100) << std::setfill(fill);
}

static auto formatWithoutDollarSign(USD usd) -> std::string {
  std::stringstream stream;
  stream << usd;
  return stream.str();
}

static auto putWithDollarSign(std::ostream &stream, USD usd) -> std::ostream & {
  stream << '$' << usd;
  return stream;
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
  stream << to_integral(month);
  return stream;
}

static auto operator<<(std::ostream &stream, const Date &date)
    -> std::ostream & {
  const auto fill{stream.fill()};
  return stream << std::setw(2) << std::setfill('0') << date.month << '/'
                << std::setw(2) << std::setfill('0') << date.day << '/'
                << date.year << std::setfill(fill);
}

CommandLineStream::CommandLineStream(std::ostream &stream) : stream{stream} {}

void CommandLineStream::show(Account &primary,
                             const std::vector<Account *> &secondaries) {
  primary.show(*this);
  for (auto *account : secondaries) {
    stream << "\n\n";
    account->show(*this);
  }
  stream << '\n';
}

void CommandLineStream::showAccountSummary(
    std::string_view name, USD balance,
    const std::vector<TransactionWithType> &transactions) {
  stream << "----" << '\n';
  stream << name << '\n';
  putWithDollarSign(stream, balance) << '\n';
  stream << '\n';
  stream << "Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description";
  for (const auto &transaction : transactions) {
    stream << '\n';
    if (transaction.type == Transaction::Type::credit) {
      stream << std::setw(12) << "";
      stream << std::setw(13);
    } else {
      stream << std::setw(25);
    }
    stream << std::left
           << formatWithoutDollarSign(transaction.transaction.amount)
           << std::right << transaction.transaction.date << std::setw(10) << ""
           << transaction.transaction.description;
  }
  stream << '\n' << "----";
}

void CommandLineStream::prompt(std::string_view s) { stream << s << ' '; }

void CommandLineStream::show(const Transaction &t, std::string_view suffix) {
  putWithDollarSign(stream, t.amount)
      << ' ' << t.date.month << '/' << t.date.day << '/' << t.date.year << ' '
      << t.description << ' ' << suffix << '\n';
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