#include "stream.hpp"
#include "parse.hpp"
#include <sstream>
#include <string>
#include <string_view>

namespace sbash64::budget {
void OutputStream::save(Account &primary,
                        const std::vector<Account *> &secondaries) {
  const auto streamGuard{ioStreamFactory.makeOutput()};
  stream = streamGuard.get();
  primary.save(*this);
  *stream << '\n';
  for (auto *account : secondaries) {
    *stream << '\n';
    account->save(*this);
    *stream << '\n';
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

void OutputStream::saveAccount(std::string_view name,
                               const std::vector<Transaction> &credits,
                               const std::vector<Transaction> &debits) {
  *stream << name << '\n';
  *stream << "credits";
  for (const auto &credit : credits) {
    *stream << '\n';
    *stream << credit.amount << ' ';
    *stream << credit.description << ' ';
    *stream << credit.date;
  }
  *stream << '\n';
  *stream << "debits";
  for (const auto &debit : debits) {
    *stream << '\n';
    *stream << debit.amount << ' ';
    *stream << debit.description << ' ';
    *stream << debit.date;
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

void InputStream::loadAccount(std::vector<Transaction> &credits,
                              std::vector<Transaction> &debits) {
  std::string line;
  getline(*stream, line);
  getline(*stream, line);
  while (line != "debits") {
    loadTransaction(*stream, line, credits);
  }
  getline(*stream, line);
  while (!line.empty()) {
    loadTransaction(*stream, line, debits);
  }
}

void InputStream::load(
    Account::Factory &factory, std::shared_ptr<Account> &primary,
    std::map<std::string, std::shared_ptr<Account>, std::less<>> &secondaries) {
  const auto streamGuard{ioStreamFactory.makeInput()};
  stream = streamGuard.get();
  std::string line;
  getline(*stream, line);
  primary = factory.make(line);
  primary->load(*this);
  while (getline(*stream, line)) {
    auto next{factory.make(line)};
    next->load(*this);
    secondaries[line] = std::move(next);
  }
}

InputStream::InputStream(IoStreamFactory &ioStreamFactory)
    : ioStreamFactory{ioStreamFactory} {}

OutputStream::OutputStream(IoStreamFactory &ioStreamFactory)
    : ioStreamFactory{ioStreamFactory} {}
} // namespace sbash64::budget
