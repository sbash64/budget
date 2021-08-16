#ifndef SBASH64_BUDGET_DOMAIN_HPP_
#define SBASH64_BUDGET_DOMAIN_HPP_

#include <cstdint>
#include <memory>
#include <string>
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
struct USD {
  std::int_least64_t cents{};

  auto operator==(const USD &) const -> bool = default;
};

constexpr auto operator+=(USD &self, USD other) -> USD & {
  self.cents += other.cents;
  return self;
}

constexpr auto operator-=(USD &self, USD other) -> USD & {
  self.cents -= other.cents;
  return self;
}

constexpr auto operator+(USD a, USD b) -> USD { return a += b; }

constexpr auto operator-(USD a, USD b) -> USD { return a -= b; }

constexpr auto operator-(USD a) -> USD { return USD{-a.cents}; }

enum class Month {
  January = 1,
  February,
  March,
  April,
  May,
  June,
  July,
  August,
  September,
  October,
  November,
  December
};

struct Date {
  int year;
  Month month;
  int day;

  auto operator==(const Date &) const -> bool = default;
};

struct Transaction {
  USD amount;
  std::string description;
  Date date;

  auto operator==(const Transaction &) const -> bool = default;
};

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
  virtual void save(std::string_view name, USD funds,
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
    virtual void notifyThatFundsAreReady(USD) = 0;
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
    virtual void notifyThatFundsHaveChanged(USD) = 0;
    virtual void notifyThatCreditHasBeenAdded(ObservableTransaction &) = 0;
    virtual void notifyThatDebitHasBeenAdded(ObservableTransaction &) = 0;
    virtual void notifyThatWillBeRemoved() = 0;
  };

  virtual void attach(Observer *) = 0;
  virtual void add(const Transaction &) {}
  virtual void verify(const Transaction &) {}
  virtual void remove(const Transaction &) {}
  virtual void rename(std::string_view) = 0;
  virtual void reduce() = 0;
  virtual auto balance() -> USD { return {}; }
  virtual void remove() = 0;
  virtual void clear() = 0;
  virtual void withdraw(USD) = 0;
  virtual void deposit(USD) = 0;
};

class IncomeAccount : public virtual Account {};

class ExpenseAccount : public virtual Account {
public:
  class Factory {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Factory);
    virtual auto make(std::string_view name)
        -> std::shared_ptr<ExpenseAccount> = 0;
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
  virtual void credit(const Transaction &) = 0;
  virtual void debit(std::string_view accountName, const Transaction &) = 0;
  virtual void removeCredit(const Transaction &) = 0;
  virtual void removeDebit(std::string_view accountName,
                           const Transaction &) = 0;
  virtual void verifyCredit(const Transaction &) = 0;
  virtual void verifyDebit(std::string_view accountName,
                           const Transaction &) = 0;
  virtual void transferTo(std::string_view accountName, USD) = 0;
  virtual void allocate(std::string_view accountName, USD) = 0;
  virtual void createAccount(std::string_view name) = 0;
  virtual void removeAccount(std::string_view name) = 0;
  virtual void renameAccount(std::string_view from, std::string_view to) = 0;
  virtual void closeAccount(std::string_view name) = 0;
  virtual void restore() = 0;
  virtual void reduce() = 0;
  virtual void save(BudgetSerialization &) = 0;
  virtual void load(BudgetDeserialization &) = 0;
};
} // namespace sbash64::budget

#endif
