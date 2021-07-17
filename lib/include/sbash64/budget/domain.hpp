#ifndef SBASH64_BUDGET_DOMAIN_HPP_
#define SBASH64_BUDGET_DOMAIN_HPP_

#include "transaction.hpp"

#include <memory>
#include <string_view>
#include <vector>

#define SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(a)                   \
  virtual ~a() = default;                                                      \
  a() = default;                                                               \
  a(const a &) = delete;                                                       \
  a(a &&) = delete;                                                            \
  auto operator=(const a &)->a & = delete;                                     \
  auto operator=(a &&)->a & = delete

namespace sbash64::budget {
struct VerifiableTransaction {
  Transaction transaction;
  bool verified{};

  auto operator==(const VerifiableTransaction &) const -> bool = default;
};

class TransactionSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(TransactionSerialization);
  virtual void save(const VerifiableTransaction &) = 0;
};

class TransactionDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void ready(const VerifiableTransaction &) = 0;
  };

  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(TransactionDeserialization);
  virtual void load(Observer &) = 0;
};

class SerializableTransaction {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(SerializableTransaction);
  virtual void save(TransactionSerialization &) = 0;
  virtual void load(TransactionDeserialization &) = 0;
};

class ObservableTransaction : public TransactionDeserialization::Observer,
                              public SerializableTransaction {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatIsVerified() = 0;
    virtual void notifyThatIs(const Transaction &) = 0;
    virtual void notifyThatWillBeRemoved() = 0;
  };

  virtual void attach(Observer *) = 0;
  virtual void initialize(const Transaction &) = 0;
  virtual void verify() = 0;
  virtual auto verifies(const Transaction &) -> bool = 0;
  virtual auto removes(const Transaction &) -> bool = 0;
  virtual void remove() = 0;
  virtual auto amount() -> USD = 0;

  class Factory {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Factory);
    virtual auto make() -> std::shared_ptr<ObservableTransaction> = 0;
  };
};

class AccountSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountSerialization);
  virtual void save(std::string_view name,
                    const std::vector<SerializableTransaction *> &credits,
                    const std::vector<SerializableTransaction *> &debits) = 0;
};

class AccountDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatCreditIsReady(TransactionDeserialization &) = 0;
    virtual void notifyThatDebitIsReady(TransactionDeserialization &) = 0;
  };

  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountDeserialization);
  virtual void load(Observer &) = 0;
};

class SerializableAccount {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(SerializableAccount);
  virtual void save(AccountSerialization &) = 0;
  virtual void load(AccountDeserialization &) = 0;
};

class Account : public AccountDeserialization::Observer,
                public SerializableAccount {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatBalanceHasChanged(USD) = 0;
    virtual void notifyThatCreditHasBeenAdded(ObservableTransaction &) = 0;
    virtual void notifyThatDebitHasBeenAdded(ObservableTransaction &) = 0;
    virtual void notifyThatWillBeRemoved() = 0;
  };

  virtual void attach(Observer *) = 0;
  virtual void credit(const Transaction &) = 0;
  virtual void debit(const Transaction &) = 0;
  virtual void removeDebit(const Transaction &) = 0;
  virtual void removeCredit(const Transaction &) = 0;
  virtual void rename(std::string_view) = 0;
  virtual void verifyDebit(const Transaction &) = 0;
  virtual void verifyCredit(const Transaction &) = 0;
  virtual void reduce(const Date &) = 0;
  virtual auto balance() -> USD = 0;
  virtual void remove() = 0;

  class Factory {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Factory);
    virtual auto make(std::string_view name) -> std::shared_ptr<Account> = 0;
  };
};

class BudgetSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(BudgetSerialization);
  virtual void save(SerializableAccount &primary,
                    const std::vector<SerializableAccount *> &secondaries) = 0;
};

class BudgetDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatPrimaryAccountIsReady(AccountDeserialization &,
                                                 std::string_view name) = 0;
    virtual void notifyThatSecondaryAccountIsReady(AccountDeserialization &,
                                                   std::string_view name) = 0;
  };
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(BudgetDeserialization);
  virtual void load(Observer &) = 0;
};

class Budget : public BudgetDeserialization::Observer {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatNewAccountHasBeenCreated(Account &,
                                                    std::string_view name) = 0;
    virtual void notifyThatTotalBalanceHasChanged(USD) = 0;
  };

  virtual void attach(Observer *) = 0;
  virtual void debit(std::string_view accountName, const Transaction &) = 0;
  virtual void removeDebit(std::string_view accountName,
                           const Transaction &) = 0;
  virtual void removeCredit(const Transaction &) = 0;
  virtual void credit(const Transaction &) = 0;
  virtual void transferTo(std::string_view accountName, USD, Date) = 0;
  virtual void removeTransfer(std::string_view accountName, USD, Date) = 0;
  virtual void removeAccount(std::string_view) = 0;
  virtual void save(BudgetSerialization &) = 0;
  virtual void load(BudgetDeserialization &) = 0;
  virtual void renameAccount(std::string_view from, std::string_view to) = 0;
  virtual void verifyDebit(std::string_view accountName,
                           const Transaction &) = 0;
  virtual void verifyCredit(const Transaction &) = 0;
  virtual void reduce(const Date &) = 0;
  virtual void createAccount(std::string_view name) = 0;
  virtual void closeAccount(std::string_view name, const Date &) = 0;
};
} // namespace sbash64::budget

#endif