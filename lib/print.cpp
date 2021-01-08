#include "print.hpp"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

namespace sbash64::budget {
static auto operator<<(std::ostream &stream, USD usd) -> std::ostream & {
  stream << usd.cents / 100 << '.' << std::setw(2) << std::setfill('0')
         << usd.cents % 100;
  return stream;
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
  stream << std::setw(2) << std::setfill('0') << date.month;
  stream << '/';
  stream << date.day;
  stream << '/';
  stream << date.year;
  return stream;
}

StreamPrinter::StreamPrinter(std::ostream &stream) : stream{stream} {}

void StreamPrinter::show(Account &primary,
                         const std::vector<Account *> &secondaries) {
  primary.show(*this);
  for (auto *account : secondaries) {
    stream << "\n\n";
    account->show(*this);
  }
  stream << '\n';
}

void StreamPrinter::showAccountSummary(
    std::string_view name, USD balance,
    const std::vector<PrintableTransaction> &transactions) {
  stream << "----" << '\n';
  stream << name << '\n';
  putWithDollarSign(stream, balance) << '\n';
  stream << '\n';
  stream << "Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description";
  for (const auto &transaction : transactions) {
    const auto formattedAmount{
        formatWithoutDollarSign(transaction.transaction.amount)};
    stream << '\n';
    if (transaction.type == Transaction::Type::credit) {
      stream << std::string(12, ' ');
      stream << std::setw(13) << std::setfill(' ') << std::left;
      stream << formattedAmount << std::right;
      // stream << std::string(13 - formattedAmount.length(), ' ');
    } else {
      stream << formattedAmount;
      stream << std::string(25 - formattedAmount.length(), ' ');
    }
    stream << transaction.transaction.date << "          "
           << transaction.transaction.description;
  }
  stream << '\n' << "----";
}
} // namespace sbash64::budget
