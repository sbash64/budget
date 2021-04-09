#ifndef SBASH64_BUDGET_COMMAND_LINE_HPP_
#define SBASH64_BUDGET_COMMAND_LINE_HPP_

#include "budget.hpp"
#include <ostream>
#include <string>
#include <string_view>

namespace sbash64::budget {
enum class Command {
  print,
  save,
  load,
  credit,
  debit,
  transfer,
  renameAccount,
  removeTransfer,
  removeDebit,
  removeCredit,
  verifyDebit,
  verifyCredit
};

constexpr auto name(Command c) -> std::string_view {
  switch (c) {
  case Command::print:
    return "print";
  case Command::save:
    return "save";
  case Command::load:
    return "load";
  case Command::credit:
    return "credit";
  case Command::debit:
    return "debit";
  case Command::renameAccount:
    return "rename";
  case Command::transfer:
    return "transfer-to";
  case Command::removeTransfer:
    return "remove-transfer";
  case Command::removeDebit:
    return "remove-debit";
  case Command::removeCredit:
    return "remove-credit";
  case Command::verifyDebit:
    return "verify-debit";
  case Command::verifyCredit:
    return "verify-credit";
  }
}

class CommandLineInterface : public virtual View {
public:
  virtual void prompt(std::string_view) = 0;
  virtual void show(std::string_view) = 0;
  virtual void show(const Transaction &) = 0;
  virtual void enumerate(const Transactions &) = 0;
};

class CommandLineInterpreter {
public:
  CommandLineInterpreter();
  void execute(Model &, CommandLineInterface &, SessionSerialization &,
               SessionDeserialization &, std::string_view);
  enum class State : int;
  enum class CommandType : int;

private:
  Date date{};
  std::string accountName;
  USD amount{};
  State state;
  CommandType commandType;
  Transaction::Type transactionType;
  Transactions unverifiedTransactions;
  Transaction unverifiedTransaction;
};

void execute(CommandLineInterpreter &, Model &, CommandLineInterface &,
             SessionSerialization &, SessionDeserialization &,
             std::string_view);

class CommandLineStream : public CommandLineInterface {
public:
  explicit CommandLineStream(std::ostream &);
  void show(Account &primary,
            const std::vector<Account *> &secondaries) override;
  void showAccountSummary(
      std::string_view name, USD balance,
      const std::vector<VerifiableTransactionWithType> &) override;
  void prompt(std::string_view) override;
  void show(const Transaction &) override;
  void show(std::string_view) override;
  void enumerate(const Transactions &) override;

private:
  std::ostream &stream;
};

auto usd(std::string_view) -> USD;
} // namespace sbash64::budget

#endif
