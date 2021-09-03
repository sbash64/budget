#include "serialization.hpp"

#include <functional>
#include <sstream>
#include <string>
#include <string_view>

namespace sbash64::budget {
ReadsBudgetFromStream::ReadsBudgetFromStream(
    IoStreamFactory &ioStreamFactory,
    AccountFromStreamFactory &accountDeserializationFactory)
    : ioStreamFactory{ioStreamFactory}, accountDeserializationFactory{
                                            accountDeserializationFactory} {}

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

void ReadsBudgetFromStream::load(Observer &observer) {
  const auto stream{ioStreamFactory.makeInput()};
  const auto accountDeserialization{
      accountDeserializationFactory.make(*stream)};
  std::string line;
  getline(*stream, line);
  observer.notifyThatIncomeAccountIsReady(*accountDeserialization, usd(line));
  while (getline(*stream, line)) {
    const auto lastSpace{line.find_last_of(' ')};
    observer.notifyThatExpenseAccountIsReady(*accountDeserialization,
                                             line.substr(0, lastSpace),
                                             usd(line.substr(lastSpace)));
  }
}

WritesBudgetToStream::WritesBudgetToStream(
    IoStreamFactory &ioStreamFactory,
    AccountToStreamFactory &accountSerializationFactory)
    : ioStreamFactory{ioStreamFactory}, accountSerializationFactory{
                                            accountSerializationFactory} {}

static auto putNewLine(std::ostream &stream) -> std::ostream & {
  return stream << '\n';
}

static auto putNewLine(const std::shared_ptr<std::ostream> &stream)
    -> std::ostream & {
  return putNewLine(*stream);
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

void WritesBudgetToStream::save(
    SerializableAccountWithFunds incomeAccountWithFunds,
    const std::vector<SerializableAccountWithFundsAndName>
        &expenseAccountsWithFunds) {
  const auto stream{ioStreamFactory.makeOutput()};
  const auto accountSerialization{accountSerializationFactory.make(*stream)};
  putNewLine(*stream << incomeAccountWithFunds.funds);
  incomeAccountWithFunds.account->save(*accountSerialization);
  for (auto accountWithFunds : expenseAccountsWithFunds) {
    putNewLine(stream);
    putNewLine(*stream << accountWithFunds.name << ' '
                       << accountWithFunds.funds);
    accountWithFunds.account->save(*accountSerialization);
  }
}

ReadsAccountFromStream::ReadsAccountFromStream(
    std::istream &stream, TransactionFromStreamFactory &factory)
    : stream{stream}, factory{factory} {}

ReadsAccountFromStream::Factory::Factory(TransactionFromStreamFactory &factory)
    : factory{factory} {}

auto ReadsAccountFromStream::Factory::make(std::istream &stream_)
    -> std::shared_ptr<AccountDeserialization> {
  return std::make_shared<ReadsAccountFromStream>(stream_, factory);
}

void ReadsAccountFromStream::load(Observer &observer) {
  const auto transactionRecordDeserialization{factory.make(stream)};
  auto next{stream.get()};
  while (stream && next != '\n') {
    stream.unget();
    observer.notifyThatIsReady(*transactionRecordDeserialization);
    next = stream.get();
  }
}

WritesAccountToStream::WritesAccountToStream(
    std::ostream &stream, TransactionToStreamFactory &factory)
    : stream{stream}, factory{factory} {}

WritesAccountToStream::Factory::Factory(TransactionToStreamFactory &factory)
    : factory{factory} {}

auto WritesAccountToStream::Factory::make(std::ostream &stream_)
    -> std::shared_ptr<AccountSerialization> {
  return std::make_shared<WritesAccountToStream>(stream_, factory);
}

void WritesAccountToStream::save(
    const std::vector<SerializableTransaction *> &transactions) {
  for (const auto &transaction : transactions) {
    transaction->save(*factory.make(stream));
    putNewLine(stream);
  }
}

ReadsTransactionFromStream::ReadsTransactionFromStream(std::istream &stream)
    : stream{stream} {}

auto ReadsTransactionFromStream::Factory::make(std::istream &stream_)
    -> std::shared_ptr<TransactionDeserialization> {
  return std::make_shared<ReadsTransactionFromStream>(stream_);
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

static void loadTransaction(
    std::string &line,
    const std::function<void(const ArchivableVerifiableTransaction &)>
        &onDeserialization) {
  std::stringstream transaction{line};
  auto verified{false};
  if (transaction.peek() == '^') {
    transaction.get();
    verified = true;
  }
  std::string next;
  transaction >> next;
  const auto amount{usd(next)};
  transaction >> next;
  std::string eventuallyDate;
  transaction >> eventuallyDate;
  std::string eventuallyEndOfLine;
  transaction >> eventuallyEndOfLine;
  std::stringstream description;
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

void ReadsTransactionFromStream::load(Observer &observer) {
  std::string line;
  getline(stream, line);
  loadTransaction(line, [&observer](const ArchivableVerifiableTransaction &t) {
    observer.ready(t);
  });
}

WritesTransactionToStream::WritesTransactionToStream(std::ostream &stream)
    : stream{stream} {}

auto WritesTransactionToStream::Factory::make(std::ostream &stream_)
    -> std::shared_ptr<TransactionSerialization> {
  return std::make_shared<WritesTransactionToStream>(stream_);
}

constexpr auto to_integral(Month e) ->
    typename std::underlying_type<Month>::type {
  return static_cast<typename std::underlying_type<Month>::type>(e);
}

static auto operator<<(std::ostream &stream, const Date &date)
    -> std::ostream & {
  return stream << to_integral(date.month) << '/' << date.day << '/'
                << date.year;
}

static void save(std::ostream &stream, const Transaction &transaction) {
  stream << transaction.amount << ' ' << transaction.description << ' '
         << transaction.date;
}

void WritesTransactionToStream::save(
    const ArchivableVerifiableTransaction &credit) {
  if (credit.verified)
    stream << '^';
  budget::save(stream, credit.transaction);
}
} // namespace sbash64::budget
