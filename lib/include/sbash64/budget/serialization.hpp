#ifndef SBASH64_BUDGET_SERIALIZATION_HPP_
#define SBASH64_BUDGET_SERIALIZATION_HPP_

#include "budget.hpp"
#include <istream>
#include <memory>
#include <ostream>

namespace sbash64::budget {
class IoStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(IoStreamFactory);
  virtual auto makeInput() -> std::shared_ptr<std::istream> = 0;
  virtual auto makeOutput() -> std::shared_ptr<std::ostream> = 0;
};

class StreamAccountSerializationFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      StreamAccountSerializationFactory);
  virtual auto make(std::ostream &)
      -> std::shared_ptr<AccountSerialization> = 0;
};

class StreamAccountDeserializationFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      StreamAccountDeserializationFactory);
  virtual auto make(std::istream &)
      -> std::shared_ptr<AccountDeserialization> = 0;
};

class StreamTransactionRecordSerializationFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      StreamTransactionRecordSerializationFactory);
  virtual auto make(std::ostream &)
      -> std::shared_ptr<TransactionRecordSerialization> = 0;
};

class StreamTransactionRecordDeserializationFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      StreamTransactionRecordDeserializationFactory);
  virtual auto make(std::istream &)
      -> std::shared_ptr<TransactionRecordDeserialization> = 0;
};

class WritesSessionToStream : public BudgetSerialization {
public:
  WritesSessionToStream(IoStreamFactory &, StreamAccountSerializationFactory &);
  void save(Account &primary,
            const std::vector<Account *> &secondaries) override;

private:
  IoStreamFactory &ioStreamFactory;
  StreamAccountSerializationFactory &accountSerializationFactory;
};

class WritesAccountToStream : public AccountSerialization {
public:
  explicit WritesAccountToStream(
      std::ostream &stream,
      StreamTransactionRecordSerializationFactory &factory);
  void save(std::string_view name,
            const std::vector<TransactionRecord *> &credits,
            const std::vector<TransactionRecord *> &debits) override;

  class Factory : public StreamAccountSerializationFactory {
  public:
    explicit Factory(StreamTransactionRecordSerializationFactory &factory)
        : factory{factory} {}

    auto make(std::ostream &stream)
        -> std::shared_ptr<AccountSerialization> override {
      return std::make_shared<WritesAccountToStream>(stream, factory);
    }

  private:
    StreamTransactionRecordSerializationFactory &factory;
  };

private:
  std::ostream &stream;
  StreamTransactionRecordSerializationFactory &factory;
};

class WritesTransactionRecordToStream : public TransactionRecordSerialization {
public:
  explicit WritesTransactionRecordToStream(std::ostream &stream);
  void save(const VerifiableTransaction &) override;

  class Factory : public StreamTransactionRecordSerializationFactory {
  public:
    auto make(std::ostream &stream)
        -> std::shared_ptr<TransactionRecordSerialization> override {
      return std::make_shared<WritesTransactionRecordToStream>(stream);
    }
  };

private:
  std::ostream &stream;
};

class ReadsSessionFromStream : public BudgetDeserialization {
public:
  ReadsSessionFromStream(IoStreamFactory &,
                         StreamAccountDeserializationFactory &);
  void load(Observer &) override;

private:
  IoStreamFactory &ioStreamFactory;
  StreamAccountDeserializationFactory &accountDeserializationFactory;
};

class ReadsAccountFromStream : public AccountDeserialization {
public:
  explicit ReadsAccountFromStream(
      std::istream &, StreamTransactionRecordDeserializationFactory &);
  void load(Observer &) override;

  class Factory : public StreamAccountDeserializationFactory {
  public:
    explicit Factory(StreamTransactionRecordDeserializationFactory &factory)
        : factory{factory} {}

    auto make(std::istream &stream)
        -> std::shared_ptr<AccountDeserialization> override {
      return std::make_shared<ReadsAccountFromStream>(stream, factory);
    }

  private:
    StreamTransactionRecordDeserializationFactory &factory;
  };

private:
  std::istream &stream;
  StreamTransactionRecordDeserializationFactory &factory;
};

class ReadsTransactionRecordFromStream
    : public TransactionRecordDeserialization {
public:
  explicit ReadsTransactionRecordFromStream(std::istream &);
  void load(Observer &) override;

private:
  std::istream &stream;
};
} // namespace sbash64::budget

#endif
