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
  auto operator=(const a &) -> a & = delete;                                   \
  auto operator=(a &&) -> a & = delete

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
};

struct ArchivableVerifiableTransaction : Transaction {
  bool verified{};
  bool archived{};

  auto operator==(const ArchivableVerifiableTransaction &) const
      -> bool = default;
};

class TransactionSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(TransactionSerialization);
  virtual void save(const ArchivableVerifiableTransaction &) = 0;
};

class TransactionDeserialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(TransactionDeserialization);
  virtual auto load() -> ArchivableVerifiableTransaction = 0;
};

class SerializableTransaction {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(SerializableTransaction);
  virtual void save(TransactionSerialization &) = 0;
};

class ObservableTransaction : public SerializableTransaction {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatIsVerified() = 0;
    virtual void notifyThatIsArchived() = 0;
    virtual void notifyThatIs(const Transaction &) = 0;
    virtual void notifyThatWillBeRemoved() = 0;
  };

  virtual void attach(Observer &) = 0;
  virtual void initialize(const Transaction &) = 0;
  virtual auto verifies(const Transaction &) -> bool = 0;
  virtual auto verified() -> bool = 0;
  virtual auto removes(const Transaction &) -> bool = 0;
  virtual void remove() = 0;
  virtual void archive() = 0;
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
  virtual void save(const std::vector<SerializableTransaction *> &,
                    USD allocated) = 0;
};

class AccountDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatIsReady(TransactionDeserialization &) = 0;
    virtual void notifyThatAllocatedIsReady(USD) = 0;
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
    virtual void notifyThatAllocationHasChanged(USD) = 0;
    virtual void notifyThatHasBeenAdded(ObservableTransaction &) = 0;
    virtual void notifyThatWillBeRemoved() = 0;
    virtual void notifyThatNameHasChanged(std::string_view) = 0;
  };

  virtual void attach(Observer &) = 0;
  virtual void add(const Transaction &) = 0;
  virtual void verify(const Transaction &) = 0;
  virtual void remove(const Transaction &) = 0;
  virtual void increaseAllocationBy(USD) = 0;
  virtual void decreaseAllocationBy(USD) = 0;
  virtual auto allocated() -> USD = 0;
  virtual auto balance() -> USD = 0;
  virtual void increaseAllocationByResolvingVerifiedTransactions() = 0;
  virtual void decreaseAllocationByResolvingVerifiedTransactions() = 0;
  virtual void remove() = 0;
  virtual void clear() = 0;
  virtual void rename(std::string_view) = 0;

  class Factory {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Factory);
    virtual auto make() -> std::shared_ptr<Account> = 0;
  };
};

struct SerializableAccountWithName {
  SerializableAccount *account{};
  std::string name;
};

class BudgetSerialization {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(BudgetSerialization);
  virtual void
  save(SerializableAccount *incomeAccount,
       const std::vector<SerializableAccountWithName> &expenseAccounts) = 0;
};

class BudgetDeserialization {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatIncomeAccountIsReady(AccountDeserialization &) = 0;
    virtual void notifyThatExpenseAccountIsReady(AccountDeserialization &,
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
    virtual void
    notifyThatExpenseAccountHasBeenCreated(Account &,
                                           std::string_view name) = 0;
    virtual void notifyThatNetIncomeHasChanged(USD) = 0;
    virtual void notifyThatHasBeenSaved() = 0;
    virtual void notifyThatHasUnsavedChanges() = 0;
  };

  virtual void attach(Observer &) = 0;
  virtual void addIncome(const Transaction &) = 0;
  virtual void addExpense(std::string_view accountName,
                          const Transaction &) = 0;
  virtual void removeIncome(const Transaction &) = 0;
  virtual void removeExpense(std::string_view accountName,
                             const Transaction &) = 0;
  virtual void verifyIncome(const Transaction &) = 0;
  virtual void verifyExpense(std::string_view accountName,
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
