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
};

constexpr auto operator==(const Date &a, const Date &b) -> bool {
  return a.year == b.year && a.month == b.month && a.day == b.day;
}

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

  enum class Type { credit, debit };
};

constexpr auto operator==(const Transaction &a, const Transaction &b) -> bool {
  return a.amount == b.amount && a.description == b.description &&
         a.date == b.date;
}

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
};

constexpr auto operator==(const VerifiableTransaction &a,
                          const VerifiableTransaction &b) -> bool {
  return a.transaction == b.transaction && a.verified == b.verified;
}

struct VerifiableTransactionWithType {
  VerifiableTransaction verifiableTransaction;
  Transaction::Type type{};
};

template <typename T>
auto debit(T &&transaction) -> VerifiableTransactionWithType {
  return {{std::forward<T>(transaction)}, Transaction::Type::debit};
}

template <typename T>
auto credit(T &&transaction) -> VerifiableTransactionWithType {
  return {{std::forward<T>(transaction)}, Transaction::Type::credit};
}

template <typename T>
auto verifiedDebit(T &&transaction) -> VerifiableTransactionWithType {
  return {{std::forward<T>(transaction), true}, Transaction::Type::debit};
}

template <typename T>
auto verifiedCredit(T &&transaction) -> VerifiableTransactionWithType {
  return {{std::forward<T>(transaction), true}, Transaction::Type::credit};
}

class View;

using Transactions = std::vector<Transaction>;
using VerifiableTransactions = std::vector<VerifiableTransaction>;

class AccountSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountSerialization);
  virtual void save(std::string_view name,
                    const VerifiableTransactions &credits,
                    const VerifiableTransactions &debits) = 0;
};

class AccountDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void
    notifyThatDebitHasBeenDeserialized(const VerifiableTransaction &) = 0;
    virtual void
    notifyThatCreditHasBeenDeserialized(const VerifiableTransaction &) = 0;
  };
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountDeserialization);
  virtual void load(Observer &) = 0;
};

class TransactionRecord {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(TransactionRecord);
  virtual void verify() = 0;

  class Factory {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Factory);
    virtual auto make(const Transaction &)
        -> std::shared_ptr<TransactionRecord> = 0;
  };
};

class Account : public AccountDeserialization::Observer {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatBalanceHasChanged(USD) = 0;
    virtual void notifyThatCreditHasBeenAdded(TransactionRecord &,
                                              const Transaction &) = 0;
    virtual void notifyThatDebitHasBeenAdded(TransactionRecord &,
                                             const Transaction &) = 0;
    virtual void notifyThatDebitHasBeenRemoved(const Transaction &) = 0;
    virtual void notifyThatCreditHasBeenRemoved(const Transaction &) = 0;
  };

  virtual void attach(Observer *) = 0;
  virtual void credit(const Transaction &) = 0;
  virtual void debit(const Transaction &) = 0;
  virtual void show(View &) = 0;
  virtual void save(AccountSerialization &) = 0;
  virtual void load(AccountDeserialization &) = 0;
  virtual void removeDebit(const Transaction &) = 0;
  virtual void removeCredit(const Transaction &) = 0;
  virtual void rename(std::string_view) = 0;
  virtual auto findUnverifiedDebits(USD amount) -> Transactions = 0;
  virtual auto findUnverifiedCredits(USD amount) -> Transactions = 0;
  virtual void verifyDebit(const Transaction &) = 0;
  virtual void verifyCredit(const Transaction &) = 0;
  virtual void reduce(const Date &) = 0;
  virtual auto balance() -> USD = 0;

  class Factory {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Factory);
    virtual auto make(std::string_view name, TransactionRecord::Factory &)
        -> std::shared_ptr<Account> = 0;
  };
};

class View {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(View);
  virtual void show(Account &primary,
                    const std::vector<Account *> &secondaries) = 0;
  virtual void showAccountSummary(
      std::string_view name, USD balance,
      const std::vector<VerifiableTransactionWithType> &transactions) = 0;
};

class SessionSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(SessionSerialization);
  virtual void save(Account &primary,
                    const std::vector<Account *> &secondaries) = 0;
};

class SessionDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatPrimaryAccountIsReady(AccountDeserialization &,
                                                 std::string_view name) = 0;
    virtual void notifyThatSecondaryAccountIsReady(AccountDeserialization &,
                                                   std::string_view name) = 0;
  };
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(SessionDeserialization);
  virtual void load(Observer &) = 0;
};

class Model : public SessionDeserialization::Observer {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatNewAccountHasBeenCreated(Account &,
                                                    std::string_view name) = 0;
    virtual void notifyThatTotalBalanceHasChanged(USD) = 0;
    virtual void notifyThatAccountHasBeenRemoved(std::string_view name) = 0;
  };

  virtual void attach(Observer *) = 0;
  virtual void debit(std::string_view accountName, const Transaction &) = 0;
  virtual void removeDebit(std::string_view accountName,
                           const Transaction &) = 0;
  virtual void removeCredit(const Transaction &) = 0;
  virtual void credit(const Transaction &) = 0;
  virtual void transferTo(std::string_view accountName, USD amount, Date) = 0;
  virtual void removeTransfer(std::string_view accountName, USD amount,
                              Date) = 0;
  virtual void removeAccount(std::string_view) = 0;
  virtual void show(View &) = 0;
  virtual void save(SessionSerialization &) = 0;
  virtual void load(SessionDeserialization &) = 0;
  virtual void renameAccount(std::string_view from, std::string_view to) = 0;
  virtual auto findUnverifiedDebits(std::string_view accountName, USD amount)
      -> Transactions = 0;
  virtual auto findUnverifiedCredits(USD amount) -> Transactions = 0;
  virtual void verifyDebit(std::string_view accountName,
                           const Transaction &) = 0;
  virtual void verifyCredit(const Transaction &) = 0;
  virtual void reduce(const Date &) = 0;
};
} // namespace sbash64::budget

#endif
