#include "serialization.hpp"

#include <sstream>
#include <string>
#include <string_view>

namespace sbash64::budget {
ReadsBudgetFromStream::ReadsBudgetFromStream(
    IoStreamFactory &ioStreamFactory,
    AccountFromStreamFactory &accountDeserializationFactory)
    : ioStreamFactory{ioStreamFactory},
      accountDeserializationFactory{accountDeserializationFactory} {}

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
  observer.notifyThatIncomeAccountIsReady(*accountDeserialization);
  std::string name;
  while (getline(*stream, name)) {
    observer.notifyThatExpenseAccountIsReady(*accountDeserialization, name);
  }
}

WritesBudgetToStream::WritesBudgetToStream(
    IoStreamFactory &ioStreamFactory,
    AccountToStreamFactory &accountSerializationFactory)
    : ioStreamFactory{ioStreamFactory},
      accountSerializationFactory{accountSerializationFactory} {}

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
    SerializableAccount *incomeAccount,
    const std::vector<SerializableAccountWithName> &expenseAccounts) {
  const auto stream{ioStreamFactory.makeOutput()};
  const auto accountSerialization{accountSerializationFactory.make(*stream)};
  incomeAccount->save(*accountSerialization);
  for (auto [account, name] : expenseAccounts) {
    putNewLine(stream);
    putNewLine(*stream << name);
    account->save(*accountSerialization);
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
  std::string allocation;
  getline(stream, allocation);
  observer.notifyThatAllocatedIsReady(usd(allocation));
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
    const std::vector<SerializableTransaction *> &transactions, USD allocated) {
  putNewLine(stream << allocated);
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

static auto loadTransaction(std::string &line)
    -> ArchivableVerifiableTransaction {
  std::stringstream transaction{line};
  auto verified{false};
  auto archived{false};
  if (transaction.peek() == '^') {
    transaction.get();
    verified = true;
  }
  if (transaction.peek() == '%') {
    transaction.get();
    verified = true;
    archived = true;
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
  return {
      {amount, description.str(), date(eventuallyDate)}, verified, archived};
}

auto ReadsTransactionFromStream::load() -> ArchivableVerifiableTransaction {
  std::string line;
  getline(stream, line);
  return loadTransaction(line);
}

WritesTransactionToStream::WritesTransactionToStream(std::ostream &stream)
    : stream{stream} {}

auto WritesTransactionToStream::Factory::make(std::ostream &stream_)
    -> std::shared_ptr<TransactionSerialization> {
  return std::make_shared<WritesTransactionToStream>(stream_);
}

constexpr auto to_integral(Month e) -> std::underlying_type_t<Month> {
  return static_cast<std::underlying_type_t<Month>>(e);
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
    const ArchivableVerifiableTransaction &transaction) {
  if (transaction.archived)
    stream << '%';
  else if (transaction.verified)
    stream << '^';
  budget::save(stream, transaction);
}
} // namespace sbash64::budget
