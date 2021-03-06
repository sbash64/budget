#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
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
  std::int_least64_t cents;
};

constexpr auto operator+(USD a, USD b) -> USD { return USD{a.cents + b.cents}; }

constexpr auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

constexpr auto operator==(USD a, USD b) -> bool { return a.cents == b.cents; }

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

constexpr auto operator<(const Date &a, const Date &b) -> bool {
  if (a.year != b.year)
    return a.year < b.year;
  if (a.month != b.month)
    return a.month < b.month;
  return a.day < b.day;
}

struct Transaction {
  USD amount;
  std::string description;
  Date date;

  auto operator==(const Transaction &) const -> bool = default;

  enum class Type { credit, debit };
};

static auto operator<(const Transaction &a, const Transaction &b) -> bool {
  if (a.date != b.date)
    return a.date < b.date;
  if (a.description != b.description)
    return a.description < b.description;
  if (a.amount != b.amount)
    return a.amount.cents < b.amount.cents;
  return false;
}

struct VerifiableTransaction {
  Transaction transaction;
  bool verified{};

  auto operator==(const VerifiableTransaction &) const -> bool = default;
};

using Transactions = std::vector<Transaction>;
using VerifiableTransactions = std::vector<VerifiableTransaction>;

class TransactionRecordSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      TransactionRecordSerialization);
  virtual void save(const VerifiableTransaction &) = 0;
};

class TransactionRecordDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void ready(const VerifiableTransaction &) = 0;
  };

  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(
      TransactionRecordDeserialization);
  virtual void load(Observer &) = 0;
};

class TransactionRecord : public TransactionRecordDeserialization::Observer {
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
  virtual void save(TransactionRecordSerialization &) = 0;
  virtual void load(TransactionRecordDeserialization &) = 0;
  virtual auto amount() -> USD = 0;

  class Factory {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Factory);
    virtual auto make() -> std::shared_ptr<TransactionRecord> = 0;
  };
};

class AccountSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountSerialization);
  virtual void save(std::string_view name,
                    const std::vector<TransactionRecord *> &credits,
                    const std::vector<TransactionRecord *> &debits) = 0;
};

class AccountDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void
    notifyThatCreditIsReady(TransactionRecordDeserialization &) = 0;
    virtual void notifyThatDebitIsReady(TransactionRecordDeserialization &) = 0;
  };

  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountDeserialization);
  virtual void load(Observer &) = 0;
};

class Account : public AccountDeserialization::Observer {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatBalanceHasChanged(USD) = 0;
    virtual void notifyThatCreditHasBeenAdded(TransactionRecord &) = 0;
    virtual void notifyThatDebitHasBeenAdded(TransactionRecord &) = 0;
    virtual void notifyThatWillBeRemoved() = 0;
  };

  virtual void attach(Observer *) = 0;
  virtual void credit(const Transaction &) = 0;
  virtual void debit(const Transaction &) = 0;
  virtual void save(AccountSerialization &) = 0;
  virtual void load(AccountDeserialization &) = 0;
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
    virtual auto make(std::string_view name, TransactionRecord::Factory &)
        -> std::shared_ptr<Account> = 0;
  };
};

class BudgetSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(BudgetSerialization);
  virtual void save(Account &primary,
                    const std::vector<Account *> &secondaries) = 0;
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
