#include "view.hpp"
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>

namespace sbash64::budget {
static auto operator<<(std::ostream &stream, USD usd) -> std::ostream & {
  const auto fill{stream.fill()};
  return stream << usd.cents / 100 << '.' << std::setw(2) << std::setfill('0')
                << std::abs(usd.cents % 100) << std::setfill(fill);
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
  const auto fill{stream.fill()};
  return stream << std::setw(2) << std::setfill('0') << date.month << '/'
                << date.day << '/' << date.year << std::setfill(fill);
}

StreamView::StreamView(std::ostream &stream) : stream{stream} {}

void StreamView::show(Account &primary,
                      const std::vector<Account *> &secondaries) {
  primary.show(*this);
  for (auto *account : secondaries) {
    stream << "\n\n";
    account->show(*this);
  }
  stream << '\n';
}

void StreamView::showAccountSummary(
    std::string_view name, USD balance,
    const std::vector<TransactionWithType> &transactions) {
  stream << "----" << '\n';
  stream << name << '\n';
  putWithDollarSign(stream, balance) << '\n';
  stream << '\n';
  stream << "Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description";
  for (const auto &transaction : transactions) {
    stream << '\n';
    if (transaction.type == Transaction::Type::credit) {
      stream << std::setw(12) << "";
      stream << std::setw(13);
    } else {
      stream << std::setw(25);
    }
    stream << std::left
           << formatWithoutDollarSign(transaction.transaction.amount)
           << std::right << transaction.transaction.date << std::setw(10) << ""
           << transaction.transaction.description;
  }
  stream << '\n' << "----";
}

void StreamView::prompt(std::string_view s) { stream << s << ' '; }
} // namespace sbash64::budget
