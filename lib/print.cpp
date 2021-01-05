#include "print.hpp"
#include <cstdio>
#include <string>

namespace sbash64::budget {
static auto formatWithoutDollarSign(USD usd) -> std::string {
  char cents[3];
  std::snprintf(cents, sizeof cents, "%.2lld", usd.cents % 100);
  return std::to_string(usd.cents / 100) + '.' + std::string{cents};
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

void StreamPrinter::print(Account &primary,
                          const std::vector<Account *> &secondaries) {
  primary.print(*this);
  for (auto *account : secondaries) {
    stream << "\n\n";
    account->print(*this);
  }
}

void StreamPrinter::printAccountSummary(
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
  stream << '\n' << "----" << '\n';
}
} // namespace sbash64::budget
