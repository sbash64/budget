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
class SessionDeserialization;
class SessionSerialization;

using Transactions = std::vector<Transaction>;

class Account {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Account);
  virtual void credit(const Transaction &) = 0;
  virtual void debit(const Transaction &) = 0;
  virtual void show(View &) = 0;
  virtual void save(SessionSerialization &) = 0;
  virtual void load(SessionDeserialization &) = 0;
  virtual void removeDebit(const Transaction &) = 0;
  virtual void removeCredit(const Transaction &) = 0;
  virtual void rename(std::string_view) = 0;
  virtual auto findUnverifiedDebits(USD amount) -> Transactions = 0;
  virtual auto findUnverifiedCredits(USD amount) -> Transactions = 0;
  virtual void verifyDebit(const Transaction &) = 0;
  virtual void verifyCredit(const Transaction &) = 0;

  class Factory {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Factory);
    virtual auto make(std::string_view name) -> std::shared_ptr<Account> = 0;
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

using VerifiableTransactions = std::vector<VerifiableTransaction>;

class SessionSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(SessionSerialization);
  virtual void save(Account &primary,
                    const std::vector<Account *> &secondaries) = 0;
  virtual void saveAccount(std::string_view name,
                           const VerifiableTransactions &credits,
                           const VerifiableTransactions &debits) = 0;
};

class SessionDeserialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(SessionDeserialization);
  virtual void loadAccount(VerifiableTransactions &credits,
                           VerifiableTransactions &debits) = 0;
  virtual void load(Account::Factory &factory,
                    std::shared_ptr<Account> &primary,
                    std::map<std::string, std::shared_ptr<Account>, std::less<>>
                        &secondaries) = 0;
};

class Model {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatDebitHasBeenAdded(std::string_view accountName,
                                             const Transaction &) = 0;
    virtual void notifyThatCreditHasBeenAdded(std::string_view accountName,
                                              const Transaction &) = 0;
  };

  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Model);
  virtual void attach(Observer *) = 0;
  virtual void debit(std::string_view accountName, const Transaction &) = 0;
  virtual void removeDebit(std::string_view accountName,
                           const Transaction &) = 0;
  virtual void removeCredit(const Transaction &) = 0;
  virtual void credit(const Transaction &) = 0;
  virtual void transferTo(std::string_view accountName, USD amount, Date) = 0;
  virtual void removeTransfer(std::string_view accountName, USD amount,
                              Date) = 0;
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
};
} // namespace sbash64::budget

#endif
