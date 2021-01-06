#include "file.hpp"
#include <string>
#include <string_view>

namespace sbash64::budget {
void File::save(Account &primary, const std::vector<Account *> &secondaries) {
  primary.save(*this);
  for (auto *account : secondaries)
    account->save(*this);
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

static auto formatDescription(std::string_view) -> std::string {}

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

File::File(std::ostream &stream) : stream{stream} {}
} // namespace sbash64::budget
