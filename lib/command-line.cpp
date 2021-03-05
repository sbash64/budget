#include "command-line.hpp"
#include "constexpr-string.hpp"
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
  readyForNewName,
  readyForUnverifiedTransactionSelection,
  readyForConfirmationOfUnverifiedTransaction
};

enum class CommandLineInterpreter::CommandType {
  addTransaction,
  removeTransaction,
  transfer,
  removeTransfer,
  renameAccount,
  verifyTransaction
};

constexpr auto matches(std::string_view s, Command c) -> bool {
  return s == name(c);
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

namespace {
class AppliesTransaction {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AppliesTransaction);
  virtual void add(Model &, const Transaction &) = 0;
  virtual void remove(Model &, const Transaction &) = 0;
};

class AppliesCredit : public AppliesTransaction {
  void add(Model &model, const Transaction &t) override { model.credit(t); }

  void remove(Model &model, const Transaction &t) override {
    model.removeCredit(t);
  }
};

class AppliesDebit : public AppliesTransaction {
public:
  explicit AppliesDebit(std::string_view accountName)
      : accountName{accountName} {}

  void add(Model &model, const Transaction &t) override {
    model.debit(accountName, t);
  }

  void remove(Model &model, const Transaction &t) override {
    model.removeDebit(accountName, t);
  }

private:
  std::string_view accountName;
};
} // namespace

static void applyTransaction(AppliesTransaction &appliesTransaction,
                             Model &model,
                             CommandLineInterpreter::CommandType commandType,
                             USD amount, const Date &date,
                             std::string_view input) {
  if (commandType == CommandLineInterpreter::CommandType::addTransaction)
    appliesTransaction.add(model, transaction(amount, date, input));
  else
    appliesTransaction.remove(model, transaction(amount, date, input));
}

static void enterTransaction(Model &model, CommandLineInterface &interface,
                             CommandLineInterpreter::State &state,
                             Transaction::Type transactionType,
                             CommandLineInterpreter::CommandType commandType,
                             USD amount, std::string_view accountName,
                             const Date &date, std::string_view input) {
  if (transactionType == Transaction::Type::credit) {
    AppliesCredit appliesTransaction;
    applyTransaction(appliesTransaction, model, commandType, amount, date,
                     input);
  } else {
    AppliesDebit appliesTransaction{accountName};
    applyTransaction(appliesTransaction, model, commandType, amount, date,
                     input);
  }
  interface.show(transaction(amount, date, input));
  state = CommandLineInterpreter::State::normal;
}

static void parseDate(Model &model, CommandLineInterface &interface,
                      CommandLineInterpreter::State &state, Date &date,
                      USD amount, std::string_view accountName,
                      CommandLineInterpreter::CommandType commandType,
                      std::string_view input) {
  switch (commandType) {
  case CommandLineInterpreter::CommandType::addTransaction:
  case CommandLineInterpreter::CommandType::removeTransaction:
    date = budget::date(input);
    state = CommandLineInterpreter::State::readyForDescription;
    interface.prompt("description [anything]");
    break;
  case CommandLineInterpreter::CommandType::transfer:
    model.transferTo(accountName, amount, budget::date(input));
    state = CommandLineInterpreter::State::normal;
    break;
  case CommandLineInterpreter::CommandType::removeTransfer:
    model.removeTransfer(accountName, amount, budget::date(input));
    state = CommandLineInterpreter::State::normal;
    break;
  case CommandLineInterpreter::CommandType::renameAccount:
  case CommandLineInterpreter::CommandType::verifyTransaction:
    break;
  }
}

static void executeFirstLineOfMultiLineCommand(
    CommandLineInterface &interface, CommandLineInterpreter::State &state,
    CommandLineInterpreter::CommandType &commandType,
    Transaction::Type &transactionType, std::string_view commandName) {
  if (matches(commandName, Command::credit) ||
      matches(commandName, Command::removeCredit) ||
      matches(commandName, Command::verifyCredit)) {
    commandType = matches(commandName, Command::credit)
                      ? CommandLineInterpreter::CommandType::addTransaction
                  : matches(commandName, Command::removeCredit)
                      ? CommandLineInterpreter::CommandType::removeTransaction
                      : CommandLineInterpreter::CommandType::verifyTransaction;
    transactionType = Transaction::Type::credit;
    state = CommandLineInterpreter::State::readyForAmount;
    interface.prompt("how much? [amount ($)]");
  } else {
    if (matches(commandName, Command::debit) ||
        matches(commandName, Command::removeDebit)) {
      commandType =
          matches(commandName, Command::debit)
              ? CommandLineInterpreter::CommandType::addTransaction
              : CommandLineInterpreter::CommandType::removeTransaction;
      transactionType = Transaction::Type::debit;
    } else if (matches(commandName, Command::renameAccount))
      commandType = CommandLineInterpreter::CommandType::renameAccount;
    else if (matches(commandName, Command::transfer))
      commandType = CommandLineInterpreter::CommandType::transfer;
    else if (matches(commandName, Command::removeTransfer))
      commandType = CommandLineInterpreter::CommandType::removeTransfer;
    else if (matches(commandName, Command::verifyDebit)) {
      commandType = CommandLineInterpreter::CommandType::verifyTransaction;
      transactionType = Transaction::Type::debit;
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
  if (matches(commandName, Command::print))
    model.show(interface);
  else if (matches(commandName, Command::save))
    model.save(serialization);
  else if (matches(commandName, Command::load))
    model.load(deserialization);
  else if (matches(commandName, Command::credit) ||
           matches(commandName, Command::debit) ||
           matches(commandName, Command::renameAccount) ||
           matches(commandName, Command::transfer) ||
           matches(commandName, Command::removeTransfer) ||
           matches(commandName, Command::removeDebit) ||
           matches(commandName, Command::removeCredit) ||
           matches(commandName, Command::verifyDebit) ||
           matches(commandName, Command::verifyCredit))
    executeFirstLineOfMultiLineCommand(interface, state, commandType,
                                       transactionType, commandName);
  else
    interface.show("unknown command \"" + commandName + '"');
}

static auto integer(std::string_view s) -> int {
  std::stringstream stream{std::string{s}};
  int integer = 0;
  stream >> integer;
  return integer;
}

static void
setAndPromptForConfirmation(Transaction &unverifiedTransaction,
                            CommandLineInterpreter::State &state,
                            CommandLineInterface &interface,
                            const Transactions &unverifiedTransactions, int i) {
  unverifiedTransaction = unverifiedTransactions.at(i);
  state = CommandLineInterpreter::State::
      readyForConfirmationOfUnverifiedTransaction;
  interface.show(unverifiedTransaction);
  interface.prompt("is the above transaction correct? [y/n]");
}

static void tryToSelectUnverfiedTransaction(
    Transaction &unverifiedTransaction, CommandLineInterpreter::State &state,
    CommandLineInterface &interface, const Transactions &unverifiedTransactions,
    std::string_view input) {
  const auto selection{integer(input) - 1};
  if (selection < 0 || selection >= unverifiedTransactions.size())
    interface.prompt("try again - which? [n]");
  else
    setAndPromptForConfirmation(unverifiedTransaction, state, interface,
                                unverifiedTransactions, selection);
}

static void parseAmount(CommandLineInterface &interface, Model &model,
                        USD &amount,
                        CommandLineInterpreter::CommandType &commandType,
                        CommandLineInterpreter::State &state,
                        Transactions &unverifiedTransactions,
                        Transaction &unverifiedTransaction,
                        const Transaction::Type transactionType,
                        std::string_view accountName, std::string_view input) {
  amount = usd(input);
  if (commandType == CommandLineInterpreter::CommandType::verifyTransaction) {
    unverifiedTransactions =
        transactionType == Transaction::Type::debit
            ? model.findUnverifiedDebits(accountName, amount)
            : model.findUnverifiedCredits(amount);
    if (unverifiedTransactions.empty()) {
      interface.show("no transactions matching amount found!");
      state = CommandLineInterpreter::State::normal;
    } else if (unverifiedTransactions.size() == 1)
      setAndPromptForConfirmation(unverifiedTransaction, state, interface,
                                  unverifiedTransactions, 0);
    else {
      state =
          CommandLineInterpreter::State::readyForUnverifiedTransactionSelection;
      interface.enumerate(unverifiedTransactions);
      interface.prompt("multiple candidates found - which? [n]");
    }
  } else {
    state = CommandLineInterpreter::State::readyForDate;
    interface.prompt("date [month day year]");
  }
}

void execute(CommandLineInterpreter &controller, Model &model,
             CommandLineInterface &interface,
             SessionSerialization &serialization,
             SessionDeserialization &deserialization, std::string_view input) {
  controller.execute(model, interface, serialization, deserialization, input);
}

CommandLineInterpreter::CommandLineInterpreter()
    : state{State::normal}, commandType{CommandType::addTransaction},
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
    if (commandType == CommandType::renameAccount) {
      state = CommandLineInterpreter::State::readyForNewName;
      interface.prompt("new name [anything]");
    } else {
      state = State::readyForAmount;
      interface.prompt("how much? [amount ($)]");
    }
    break;
  case State::readyForAmount:
    parseAmount(interface, model, amount, commandType, state,
                unverifiedTransactions, unverifiedTransaction, transactionType,
                accountName, input);
    break;
  case State::readyForDate:
    return parseDate(model, interface, state, date, amount, accountName,
                     commandType, input);
  case State::readyForDescription:
    return enterTransaction(model, interface, state, transactionType,
                            commandType, amount, accountName, date, input);
  case State::readyForNewName:
    model.renameAccount(accountName, input);
    state = State::normal;
    break;
  case State::readyForUnverifiedTransactionSelection:
    tryToSelectUnverfiedTransaction(unverifiedTransaction, state, interface,
                                    unverifiedTransactions, input);
    break;
  case State::readyForConfirmationOfUnverifiedTransaction:
    if (transactionType == Transaction::Type::debit)
      model.verifyDebit(accountName, unverifiedTransaction);
    else
      model.verifyCredit(unverifiedTransaction);
    state = State::normal;
    break;
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

static auto formatWithoutDollarSign(const VerifiableTransaction &transaction)
    -> std::string {
  std::stringstream stream;
  if (!transaction.verified)
    stream << '*';
  stream << transaction.transaction.amount;
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

auto operator<<(std::ostream &stream, const Date &date) -> std::ostream & {
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
    const std::vector<VerifiableTransactionWithType> &transactions) {
  putNewLine(stream << "----");
  putNewLine(stream << name);
  putNewLine(putWithDollarSign(stream, balance));
  putNewLine(stream);
  constexpr const std::array<char, 10> debitHeading{"Debit ($)"};
  constexpr const std::array<char, 11> creditHeading{"Credit ($)"};
  constexpr auto spacesBetweenHeadings{3};
  stream << concatenate(
                debitHeading, std::array<char, 4>{"   "}, creditHeading,
                std::array<char, 35>{"   Date (mm/dd/yyyy)   Description"})
                .data();
  for (const auto &transaction : transactions) {
    putNewLine(stream);
    constexpr auto spacesBeforeDate{length(debitHeading) +
                                    length(creditHeading) +
                                    2 * spacesBetweenHeadings};
    auto transactionWidth{length(debitHeading)};
    auto remainingSpaces{spacesBeforeDate - transactionWidth};
    if (transaction.type == Transaction::Type::credit) {
      transactionWidth = length(creditHeading);
      remainingSpaces = spacesBetweenHeadings;
      const auto spaces{spacesBeforeDate - transactionWidth - remainingSpaces};
      putSpaces(stream, spaces);
    }
    putSpaces(putSpaces(stream << std::setw(transactionWidth) << std::right
                               << formatWithoutDollarSign(
                                      transaction.verifiableTransaction),
                        remainingSpaces)
                  << std::right
                  << transaction.verifiableTransaction.transaction.date,
              10)
        << transaction.verifiableTransaction.transaction.description;
  }
  putNewLine(stream) << "----";
}

static auto putSpace(std::ostream &stream) -> std::ostream & {
  return stream << ' ';
}

void CommandLineStream::prompt(std::string_view s) { putSpace(stream << s); }

static void put(std::ostream &stream, const Transaction &t) {
  putNewLine(putSpace(putSpace(putWithDollarSign(stream, t.amount))
                      << t.date.month << '/' << t.date.day << '/'
                      << t.date.year)
             << t.description);
}

void CommandLineStream::show(const Transaction &t) { put(stream, t); }

void CommandLineStream::enumerate(const Transactions &transactions) {
  for (int i{0}; i < transactions.size(); ++i)
    put(stream << '[' << i + 1 << "] ", transactions.at(i));
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