#include "serialization.hpp"
#include <sstream>
#include <string>
#include <string_view>

namespace sbash64::budget {
void WritesSessionToStream::save(Account &primary,
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

void WritesSessionToStream::saveAccount(std::string_view name,
                                        const VerifiableTransactions &credits,
                                        const VerifiableTransactions &debits) {
  *stream << name << '\n';
  *stream << "credits";
  for (const auto &credit : credits) {
    *stream << '\n';
    if (credit.verified)
      *stream << '^';
    *stream << credit.transaction.amount << ' ';
    *stream << credit.transaction.description << ' ';
    *stream << credit.transaction.date;
  }
  *stream << '\n';
  *stream << "debits";
  for (const auto &debit : debits) {
    *stream << '\n';
    if (debit.verified)
      *stream << '^';
    *stream << debit.transaction.amount << ' ';
    *stream << debit.transaction.description << ' ';
    *stream << debit.transaction.date;
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

static auto usd(std::string_view s) -> USD {
  USD usd{};
  std::istringstream stream{std::string{s}};
  stream >> usd.cents;
  usd.cents *= 100;
  if (stream.get() == '.') {
    int cents = 0;
    stream >> cents;
    usd.cents += cents;
  }
  return usd;
}

static void loadTransaction(std::istream &input, std::string &line,
                            VerifiableTransactions &transactions) {
  std::stringstream transaction{line};
  std::string next;
  std::string eventuallyDate;
  std::string eventuallyEndOfLine;
  auto verified{false};
  if (transaction.peek() == '^') {
    transaction.get();
    verified = true;
  }
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
      {amount, description.str(), date(eventuallyDate), verified});
  getline(input, line);
}

void ReadsSessionFromStream::loadAccount(VerifiableTransactions &credits,
                                         VerifiableTransactions &debits) {
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

void ReadsSessionFromStream::load(
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

ReadsSessionFromStream::ReadsSessionFromStream(IoStreamFactory &ioStreamFactory)
    : ioStreamFactory{ioStreamFactory} {}

WritesSessionToStream::WritesSessionToStream(IoStreamFactory &ioStreamFactory)
    : ioStreamFactory{ioStreamFactory} {}
} // namespace sbash64::budget
