#include "serialization.hpp"
#include <functional>
#include <sstream>
#include <string>
#include <string_view>

namespace sbash64::budget {
void WritesSessionToStream::save(Account &primary,
                                 const std::vector<Account *> &secondaries) {
  const auto stream{ioStreamFactory.makeOutput()};
  const auto accountSerialization{accountSerializationFactory.make(*stream)};
  primary.save(*accountSerialization);
  *stream << '\n';
  for (auto *account : secondaries) {
    *stream << '\n';
    account->save(*accountSerialization);
    *stream << '\n';
  }
}

static auto operator<<(std::ostream &stream, USD amount) -> std::ostream & {
  stream << amount.cents / 100;
  const auto leftoverCents{amount.cents % 100};
  if (leftoverCents != 0) {
    stream << '.';
    if (leftoverCents < 10)
      stream << '0';
    stream << leftoverCents;
  }
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

void WritesAccountToStream::save(
    std::string_view name, const std::vector<TransactionRecord *> &credits,
    const std::vector<TransactionRecord *> &debits) {
  stream << name << '\n';
  stream << "credits";
  for (const auto &credit : credits) {
    stream << '\n';
    const auto transactionRecordSerialization{factory.make(stream)};
    credit->save(*transactionRecordSerialization);
  }
  stream << '\n';
  stream << "debits";
  for (const auto &debit : debits) {
    stream << '\n';
    const auto transactionRecordSerialization{factory.make(stream)};
    debit->save(*transactionRecordSerialization);
  }
}

void WritesTransactionRecordToStream::save(
    const VerifiableTransaction &credit) {
  if (credit.verified)
    stream << '^';
  stream << credit.transaction.amount << ' ';
  stream << credit.transaction.description << ' ';
  stream << credit.transaction.date;
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

static void
loadTransaction(std::istream &input, std::string &line,
                const std::function<void(const VerifiableTransaction &)>
                    &onDeserialization) {
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
  onDeserialization(
      {{amount, description.str(), date(eventuallyDate)}, verified});
}

void ReadsTransactionRecordFromStream::load(Observer &observer) {
  std::string line;
  getline(stream, line);
  loadTransaction(stream, line,
                  [&](const VerifiableTransaction &t) { observer.ready(t); });
}

void ReadsAccountFromStream::load(Observer &observer) {
  const auto transactionRecordDeserialization{factory.make(stream)};
  std::string line;
  getline(stream, line);
  while (stream.peek() != 'd') {
    observer.notifyThatCreditIsReady(*transactionRecordDeserialization);
  }
  getline(stream, line);
  auto next{stream.get()};
  while (stream && next != '\n') {
    stream.unget();
    observer.notifyThatDebitIsReady(*transactionRecordDeserialization);
    next = stream.get();
  }
  getline(stream, line);
}

void ReadsSessionFromStream::load(Observer &observer) {
  const auto stream{ioStreamFactory.makeInput()};
  const auto accountDeserialization{
      accountDeserializationFactory.make(*stream)};
  std::string line;
  getline(*stream, line);
  observer.notifyThatPrimaryAccountIsReady(*accountDeserialization, line);
  while (getline(*stream, line))
    observer.notifyThatSecondaryAccountIsReady(*accountDeserialization, line);
}

ReadsSessionFromStream::ReadsSessionFromStream(
    IoStreamFactory &ioStreamFactory,
    StreamAccountDeserializationFactory &accountDeserializationFactory)
    : ioStreamFactory{ioStreamFactory}, accountDeserializationFactory{
                                            accountDeserializationFactory} {}

WritesSessionToStream::WritesSessionToStream(
    IoStreamFactory &ioStreamFactory,
    StreamAccountSerializationFactory &accountSerializationFactory)
    : ioStreamFactory{ioStreamFactory}, accountSerializationFactory{
                                            accountSerializationFactory} {}

ReadsAccountFromStream::ReadsAccountFromStream(
    std::istream &stream,
    StreamTransactionRecordDeserializationFactory &factory)
    : stream{stream}, factory{factory} {}

WritesAccountToStream::WritesAccountToStream(
    std::ostream &stream, StreamTransactionRecordSerializationFactory &factory)
    : stream{stream}, factory{factory} {}

ReadsTransactionRecordFromStream::ReadsTransactionRecordFromStream(
    std::istream &stream)
    : stream{stream} {}
} // namespace sbash64::budget
