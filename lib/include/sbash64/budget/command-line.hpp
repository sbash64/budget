#ifndef SBASH64_BUDGET_COMMAND_LINE_HPP_
#define SBASH64_BUDGET_COMMAND_LINE_HPP_

#include "budget.hpp"
#include <string>
#include <string_view>

namespace sbash64::budget {
class CommandLineInterface : public virtual View {
public:
  virtual void prompt(std::string_view) = 0;
  virtual void show(const Transaction &t, std::string_view suffix) = 0;
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
};

void execute(CommandLineInterpreter &, Model &, CommandLineInterface &,
             SessionSerialization &, SessionDeserialization &,
             std::string_view);

auto format(USD) -> std::string;

class CommandLineStream : public CommandLineInterface {
public:
  explicit CommandLineStream(std::ostream &);
  void show(Account &primary,
            const std::vector<Account *> &secondaries) override;
  void showAccountSummary(std::string_view name, USD balance,
                          const std::vector<TransactionWithType> &) override;
  void prompt(std::string_view) override;
  void show(const Transaction &, std::string_view suffix) override;

private:
  std::ostream &stream;
};

auto usd(std::string_view) -> USD;
} // namespace sbash64::budget

#endif
