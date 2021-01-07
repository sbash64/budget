#include "file.hpp"
#include "parse.hpp"
#include <sstream>
#include <string>
#include <string_view>

namespace sbash64::budget {
void File::save(Account &primary, const std::vector<Account *> &secondaries) {
  primary.save(*this);
  for (auto *account : secondaries) {
    stream << "\n\n";
    account->save(*this);
  }
  stream << '\n';
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

void File::saveAccount(std::string_view name,
                       const std::vector<Transaction> &credits,
                       const std::vector<Transaction> &debits) {
  stream << name << '\n';
  stream << "credits";
  for (const auto &credit : credits) {
    stream << '\n';
    stream << credit.amount << ' ';
    stream << credit.description << ' ';
    stream << credit.date;
  }
  stream << '\n';
  stream << "debits";
  for (const auto &debit : debits) {
    stream << '\n';
    stream << debit.amount << ' ';
    stream << debit.description << ' ';
    stream << debit.date;
  }
}

static auto asDate(std::string_view s) -> Date {
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

static void f(std::istream &input, std::string &line,
              std::vector<Transaction> &transactions) {
  std::stringstream transaction{line};
  std::string word;
  transaction >> word;
  auto amount{usd(word)};
  std::stringstream description;
  transaction >> word;
  std::string nextWord;
  transaction >> nextWord;
  auto first{true};
  while (transaction) {
    if (!first)
      description << ' ';
    description << word;
    word = nextWord;
    transaction >> nextWord;
    first = false;
  }
  auto date{asDate(word)};
  transactions.push_back(Transaction{amount, description.str(), date});
  getline(input, line);
}

void File::loadAccount(std::vector<Transaction> &credits,
                       std::vector<Transaction> &debits) {
  std::string line;
  getline(input, line);
  getline(input, line);
  while (line != "debits") {
    f(input, line, credits);
  }
  getline(input, line);
  while (!line.empty()) {
    f(input, line, debits);
  }
}

File::File(std::istream &input, std::ostream &stream)
    : input{input}, stream{stream} {}
} // namespace sbash64::budget
