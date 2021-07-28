#ifndef SBASH64_BUDGET_SERIALIZATION_HPP_
#define SBASH64_BUDGET_SERIALIZATION_HPP_

#include "domain.hpp"

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

class AccountToStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountToStreamFactory);
  virtual auto make(std::ostream &)
      -> std::shared_ptr<AccountSerialization> = 0;
};

class AccountFromStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountFromStreamFactory);
  virtual auto make(std::istream &)
      -> std::shared_ptr<AccountDeserialization> = 0;
};

class TransactionToStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(TransactionToStreamFactory);
  virtual auto make(std::ostream &)
      -> std::shared_ptr<TransactionSerialization> = 0;
};

class TransactionFromStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      TransactionFromStreamFactory);
  virtual auto make(std::istream &)
      -> std::shared_ptr<TransactionDeserialization> = 0;
};

class WritesBudgetToStream : public BudgetSerialization {
public:
  WritesBudgetToStream(IoStreamFactory &, AccountToStreamFactory &);
  void save(SerializableAccount &primary,
            const std::vector<SerializableAccount *> &secondaries) override;

private:
  IoStreamFactory &ioStreamFactory;
  AccountToStreamFactory &accountSerializationFactory;
};

class WritesAccountToStream : public AccountSerialization {
public:
  explicit WritesAccountToStream(std::ostream &, TransactionToStreamFactory &);
  void save(std::string_view name, USD funds,
            const std::vector<SerializableTransaction *> &credits,
            const std::vector<SerializableTransaction *> &debits) override;

  class Factory : public AccountToStreamFactory {
  public:
    explicit Factory(TransactionToStreamFactory &);
    auto make(std::ostream &) -> std::shared_ptr<AccountSerialization> override;

  private:
    TransactionToStreamFactory &factory;
  };

private:
  std::ostream &stream;
  TransactionToStreamFactory &factory;
};

class WritesTransactionToStream : public TransactionSerialization {
public:
  explicit WritesTransactionToStream(std::ostream &);
  void save(const VerifiableTransaction &) override;

  class Factory : public TransactionToStreamFactory {
  public:
    auto make(std::ostream &)
        -> std::shared_ptr<TransactionSerialization> override;
  };

private:
  std::ostream &stream;
};

class ReadsBudgetFromStream : public BudgetDeserialization {
public:
  ReadsBudgetFromStream(IoStreamFactory &, AccountFromStreamFactory &);
  void load(Observer &) override;

private:
  IoStreamFactory &ioStreamFactory;
  AccountFromStreamFactory &accountDeserializationFactory;
};

class ReadsAccountFromStream : public AccountDeserialization {
public:
  explicit ReadsAccountFromStream(std::istream &,
                                  TransactionFromStreamFactory &);
  void load(Observer &) override;

  class Factory : public AccountFromStreamFactory {
  public:
    explicit Factory(TransactionFromStreamFactory &);
    auto make(std::istream &)
        -> std::shared_ptr<AccountDeserialization> override;

  private:
    TransactionFromStreamFactory &factory;
  };

private:
  std::istream &stream;
  TransactionFromStreamFactory &factory;
};

class ReadsTransactionFromStream : public TransactionDeserialization {
public:
  explicit ReadsTransactionFromStream(std::istream &);
  void load(Observer &) override;

  class Factory : public TransactionFromStreamFactory {
  public:
    auto make(std::istream &)
        -> std::shared_ptr<TransactionDeserialization> override;
  };

private:
  std::istream &stream;
};
} // namespace sbash64::budget

#endif
