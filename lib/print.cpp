#include "print.hpp"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

namespace sbash64::budget {
static auto formatWithoutDollarSign(USD usd) -> std::string {
  std::stringstream stream;
  stream << usd.cents / 100 << '.' << std::setw(2) << std::setfill('0')
         << usd.cents % 100;
  return stream.str();
}

static auto format_(USD usd) -> std::string {
  return '$' + formatWithoutDollarSign(usd);
}

auto format(USD usd) -> std::string { return format_(usd); }

constexpr auto to_integral(Month e) ->
    typename std::underlying_type<Month>::type {
  return static_cast<typename std::underlying_type<Month>::type>(e);
}

static auto operator<<(std::ostream &stream, const Month &month)
    -> std::ostream & {
  stream << to_integral(month);
  return stream;
}

static auto format(const Date &date) -> std::string {
  std::stringstream stream;
  stream << std::setw(2) << std::setfill('0') << date.month;
  stream << '/';
  stream << date.day;
  stream << '/';
  stream << date.year;
  return stream.str();
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
  stream << format_(balance) << '\n';
  stream << '\n';
  stream << "Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description";
  for (const auto &transaction : transactions) {
    const auto formattedAmount{
        formatWithoutDollarSign(transaction.transaction.amount)};
    stream << '\n';
    auto extraSpaces{0};
    if (transaction.type == Transaction::Type::credit)
      extraSpaces = 12;
    stream << std::string(extraSpaces, ' ');
    stream << formattedAmount;
    stream << std::string(25 - formattedAmount.length() - extraSpaces, ' ')
           << format(transaction.transaction.date) << "          "
           << transaction.transaction.description;
  }
  stream << '\n' << "----";
}
} // namespace sbash64::budget
