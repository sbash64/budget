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

class ObservableTransactionToStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      ObservableTransactionToStreamFactory);
  virtual auto make(std::ostream &)
      -> std::shared_ptr<TransactionSerialization> = 0;
};

class ObservableTransactionFromStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      ObservableTransactionFromStreamFactory);
  virtual auto make(std::istream &)
      -> std::shared_ptr<TransactionDeserialization> = 0;
};

class WritesBudgetToStream : public BudgetSerialization {
public:
  WritesBudgetToStream(IoStreamFactory &, AccountToStreamFactory &);
  void save(Account &primary,
            const std::vector<Account *> &secondaries) override;

private:
  IoStreamFactory &ioStreamFactory;
  AccountToStreamFactory &accountSerializationFactory;
};

class WritesAccountToStream : public AccountSerialization {
public:
  explicit WritesAccountToStream(std::ostream &,
                                 ObservableTransactionToStreamFactory &);
  void save(std::string_view name,
            const std::vector<ObservableTransaction *> &credits,
            const std::vector<ObservableTransaction *> &debits) override;

  class Factory : public AccountToStreamFactory {
  public:
    explicit Factory(ObservableTransactionToStreamFactory &);
    auto make(std::ostream &) -> std::shared_ptr<AccountSerialization> override;

  private:
    ObservableTransactionToStreamFactory &factory;
  };

private:
  std::ostream &stream;
  ObservableTransactionToStreamFactory &factory;
};

class WritesObservableTransactionToStream : public TransactionSerialization {
public:
  explicit WritesObservableTransactionToStream(std::ostream &);
  void save(const VerifiableTransaction &) override;

  class Factory : public ObservableTransactionToStreamFactory {
  public:
    auto make(std::ostream &)
        -> std::shared_ptr<TransactionSerialization> override;
  };

private:
  std::ostream &stream;
};

class ReadsSessionFromStream : public BudgetDeserialization {
public:
  ReadsSessionFromStream(IoStreamFactory &, AccountFromStreamFactory &);
  void load(Observer &) override;

private:
  IoStreamFactory &ioStreamFactory;
  AccountFromStreamFactory &accountDeserializationFactory;
};

class ReadsAccountFromStream : public AccountDeserialization {
public:
  explicit ReadsAccountFromStream(std::istream &,
                                  ObservableTransactionFromStreamFactory &);
  void load(Observer &) override;

  class Factory : public AccountFromStreamFactory {
  public:
    explicit Factory(ObservableTransactionFromStreamFactory &);
    auto make(std::istream &)
        -> std::shared_ptr<AccountDeserialization> override;

  private:
    ObservableTransactionFromStreamFactory &factory;
  };

private:
  std::istream &stream;
  ObservableTransactionFromStreamFactory &factory;
};

class ReadsTransactionFromStream : public TransactionDeserialization {
public:
  explicit ReadsTransactionFromStream(std::istream &);
  void load(Observer &) override;

  class Factory : public ObservableTransactionFromStreamFactory {
  public:
    auto make(std::istream &)
        -> std::shared_ptr<TransactionDeserialization> override;
  };

private:
  std::istream &stream;
};
} // namespace sbash64::budget

#endif
