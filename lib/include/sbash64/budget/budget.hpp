#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include <cstdint>
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

inline auto operator+(USD a, USD b) -> USD { return USD{a.cents + b.cents}; }

inline auto operator-(USD a, USD b) -> USD { return USD{a.cents - b.cents}; }

inline auto operator==(USD a, USD b) -> bool { return a.cents == b.cents; }

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

inline auto operator==(const Date &a, const Date &b) -> bool {
  return a.year == b.year && a.month == b.month && a.day == b.day;
}

inline auto operator<(const Date &a, const Date &b) -> bool {
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

inline auto operator==(const Transaction &a, const Transaction &b) -> bool {
  return a.amount == b.amount && a.description == b.description &&
         a.date == b.date;
}

struct PrintableTransaction {
  Transaction transaction;
  Transaction::Type type{};
};

inline auto printableDebit(Transaction transaction) -> PrintableTransaction {
  return {std::move(transaction), Transaction::Type::debit};
}

inline auto printableCredit(Transaction transaction) -> PrintableTransaction {
  return {std::move(transaction), Transaction::Type::credit};
}

class View;

class PersistentMemory;

class Account {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Account);
  virtual void credit(const Transaction &) = 0;
  virtual void debit(const Transaction &) = 0;
  virtual void show(View &) = 0;
  virtual void save(PersistentMemory &) = 0;
};

class AccountFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(AccountFactory);
  virtual auto make(std::string_view name) -> std::shared_ptr<Account> = 0;
};

class View {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(View);
  virtual void show(Account &primary,
                    const std::vector<Account *> &secondaries) = 0;
  virtual void
  showAccountSummary(std::string_view name, USD balance,
                     const std::vector<PrintableTransaction> &transactions) = 0;
};

class PersistentMemory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(PersistentMemory);
  virtual void save(Account &primary,
                    const std::vector<Account *> &secondaries) = 0;
  virtual void saveAccount(std::string_view name,
                           const std::vector<Transaction> &credits,
                           const std::vector<Transaction> &debits) = 0;
  virtual void loadAccount(std::string_view name,
                           std::vector<Transaction> &credits,
                           std::vector<Transaction> &debits) {}
};

class Model {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Model);
  virtual void debit(std::string_view accountName, const Transaction &) = 0;
  virtual void credit(const Transaction &) = 0;
  virtual void transferTo(std::string_view accountName, USD amount, Date) = 0;
  virtual void show(View &) = 0;
  virtual void save(PersistentMemory &) = 0;
  virtual void load(PersistentMemory &) {}
};
} // namespace sbash64::budget

#endif
