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

class WritesSessionToStream : public SessionSerialization {
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
  class Factory : public StreamAccountSerializationFactory {
  public:
    auto make(std::ostream &stream)
        -> std::shared_ptr<AccountSerialization> override {
      return std::make_shared<WritesAccountToStream>(stream);
    }
  };
  explicit WritesAccountToStream(std::ostream &stream);
  void save(std::string_view name, const VerifiableTransactions &credits,
            const VerifiableTransactions &debits) override;

private:
  std::ostream &stream;
};

class ReadsSessionFromStream : public SessionDeserialization {
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
  class Factory : public StreamAccountDeserializationFactory {
  public:
    auto make(std::istream &stream)
        -> std::shared_ptr<AccountDeserialization> override {
      return std::make_shared<ReadsAccountFromStream>(stream);
    }
  };
  explicit ReadsAccountFromStream(std::istream &);
  void load(Observer &) override;

private:
  std::istream &stream;
};
} // namespace sbash64::budget

#endif
