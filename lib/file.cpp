#include "file.hpp"
#include "parse.hpp"
#include <sstream>
#include <string>
#include <string_view>

namespace sbash64::budget {
void PersistentStreams::save(Account &primary,
                             const std::vector<Account *> &secondaries) {
  primary.save(*this);
  output << '\n';
  for (auto *account : secondaries) {
    output << '\n';
    account->save(*this);
    output << '\n';
  }
}

static auto operator<<(std::ostream &stream, USD amount) -> std::ostream & {
  stream << amount.cents / 100;
  const auto leftoverCents{amount.cents % 100};
  if (leftoverCents != 0)
    stream << '.' << leftoverCents;
  return stream;
}

constexpr auto to_integral(Month e) ->
    typename std::underlying_type<Month>::type {
  return static_cast<typename std::underlying_type<Month>::type>(e);
}

static auto operator<<(std::ostream &stream, const Date &date)
    -> std::ostream & {
  stream << to_integral(date.month) << '/';
  stream << date.day << '/';
  stream << date.year;
  return stream;
}

void PersistentStreams::saveAccount(std::string_view name,
                                    const std::vector<Transaction> &credits,
                                    const std::vector<Transaction> &debits) {
  output << name << '\n';
  output << "credits";
  for (const auto &credit : credits) {
    output << '\n';
    output << credit.amount << ' ';
    output << credit.description << ' ';
    output << credit.date;
  }
  output << '\n';
  output << "debits";
  for (const auto &debit : debits) {
    output << '\n';
    output << debit.amount << ' ';
    output << debit.description << ' ';
    output << debit.date;
  }
}

static auto date(std::string_view s) -> Date {
  std::stringstream stream{std::string{s}};
  int month = 0;
  stream >> month;
  stream.get();
  int day = 0;
  stream >> day;
  stream.get();
  int year = 0;
  stream >> year;
  return Date{year, Month{month}, day};
}

static void loadTransaction(std::istream &input, std::string &line,
                            std::vector<Transaction> &transactions) {
  std::stringstream transaction{line};
  std::string next;
  std::string eventuallyDate;
  std::string eventuallyEndOfLine;
  transaction >> next;
  const auto amount{usd(next)};
  std::stringstream description;
  transaction >> next;
  transaction >> eventuallyDate;
  transaction >> eventuallyEndOfLine;
  while (transaction) {
    description << next << ' ';
    next = eventuallyDate;
    eventuallyDate = eventuallyEndOfLine;
    transaction >> eventuallyEndOfLine;
  }
  description << next;
  transactions.push_back(
      Transaction{amount, description.str(), date(eventuallyDate)});
  getline(input, line);
}

void PersistentStreams::loadAccount(std::vector<Transaction> &credits,
                                    std::vector<Transaction> &debits) {
  std::string line;
  getline(input, line);
  getline(input, line);
  while (line != "debits") {
    loadTransaction(input, line, credits);
  }
  getline(input, line);
  while (!line.empty()) {
    loadTransaction(input, line, debits);
  }
}

void PersistentStreams::load(
    Account::Factory &factory, std::shared_ptr<Account> &primary,
    std::map<std::string, std::shared_ptr<Account>, std::less<>> &secondaries) {
  std::string line;
  getline(input, line);
  primary = factory.make(line);
  primary->load(*this);
  while (getline(input, line)) {
    auto next{factory.make(line)};
    next->load(*this);
    secondaries[line] = std::move(next);
  }
}

PersistentStreams::PersistentStreams(std::istream &input, std::ostream &stream)
    : input{input}, output{stream} {}
} // namespace sbash64::budget
