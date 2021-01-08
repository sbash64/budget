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

static auto format(const Date &date) -> std::string {
  char month[3];
  char day[3];
  std::snprintf(month, sizeof month, "%.2d", date.month);
  std::snprintf(day, sizeof day, "%.2d", date.day);
  return std::string{month} + '/' + std::string{day} + '/' +
         std::to_string(date.year);
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
