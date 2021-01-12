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
class InputPersistentMemory;
class OutputPersistentMemory;

class Account {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Account);
  virtual void credit(const Transaction &) = 0;
  virtual void debit(const Transaction &) = 0;
  virtual void show(View &) = 0;
  virtual void save(OutputPersistentMemory &) = 0;
  virtual void load(InputPersistentMemory &) = 0;
  virtual void removeDebit(const Transaction &) = 0;
  virtual void removeCredit(const Transaction &) = 0;

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
  virtual void
  showAccountSummary(std::string_view name, USD balance,
                     const std::vector<PrintableTransaction> &transactions) = 0;
};

class OutputPersistentMemory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(OutputPersistentMemory);
  virtual void save(Account &primary,
                    const std::vector<Account *> &secondaries) = 0;
  virtual void saveAccount(std::string_view name,
                           const std::vector<Transaction> &credits,
                           const std::vector<Transaction> &debits) = 0;
};

class InputPersistentMemory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(InputPersistentMemory);
  virtual void loadAccount(std::vector<Transaction> &credits,
                           std::vector<Transaction> &debits) = 0;
  virtual void load(Account::Factory &factory,
                    std::shared_ptr<Account> &primary,
                    std::map<std::string, std::shared_ptr<Account>, std::less<>>
                        &secondaries) = 0;
};

class Model {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Model);
  virtual void debit(std::string_view accountName, const Transaction &) = 0;
  virtual void credit(const Transaction &) = 0;
  virtual void transferTo(std::string_view accountName, USD amount, Date) = 0;
  virtual void show(View &) = 0;
  virtual void save(OutputPersistentMemory &) = 0;
  virtual void load(InputPersistentMemory &) = 0;
};
} // namespace sbash64::budget

#endif
