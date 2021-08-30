#include "budget.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"

#include <sbash64/budget/budget.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

#include <gsl/gsl>

#include <functional>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

namespace sbash64::budget {
namespace {
class AccountDeserializationStub : public AccountDeserialization {
public:
  void load(Observer &) override {}
};

class AccountStub : public virtual Account {
public:
  void clear() override { cleared_ = true; }

  void setBalance(USD b) { balance_ = b; }

  void attach(Observer *) override {}

  void save(AccountSerialization &) override {}

  void load(AccountDeserialization &a) override { deserialization_ = &a; }

  auto deserialization() -> const AccountDeserialization * {
    return deserialization_;
  }

  auto newName() -> std::string { return newName_; }

  void rename(std::string_view s) override { newName_ = s; }

  void notifyThatFundsAreReady(USD) override {}

  void reduce() override { reduced_ = true; }

  [[nodiscard]] auto reduced() const -> bool { return reduced_; }

  auto balance() -> USD override { return balance_; }

  void remove() override { removed_ = true; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

  [[nodiscard]] auto cleared() const -> bool { return cleared_; }

  void withdraw(USD usd) override {}

  void deposit(USD usd) override {}

  void notifyThatIsReady(TransactionDeserialization &) override {}

  void add(const Transaction &t) override { addedTransaction_ = t; }

  void remove(const Transaction &t) override {
    transactionRemoved_ = true;
    removedTransaction_ = t;
  }

  [[nodiscard]] auto transactionRemoved() const -> bool {
    return transactionRemoved_;
  }

  auto verifiedTransaction() -> Transaction { return verifiedTransaction_; }

  void verify(const Transaction &t) override { verifiedTransaction_ = t; }

  auto addedTransaction() -> Transaction { return addedTransaction_; }

  auto removedTransaction() -> Transaction { return removedTransaction_; }

private:
  Transaction verifiedTransaction_;
  Transaction addedTransaction_;
  Transaction removedTransaction_;
  bool transactionRemoved_{};
  std::string newName_;
  const AccountDeserialization *deserialization_{};
  USD balance_{};
  bool removed_{};
  bool cleared_{};
  bool reduced_{};
};

class AccountFactoryStub : public Account::Factory {
public:
  void add(std::shared_ptr<Account> account, std::string_view name) {
    accounts[std::string{name}] = std::move(account);
  }

  auto name() -> std::string { return name_; }

  auto make(std::string_view s) -> std::shared_ptr<Account> override {
    name_ = s;
    return accounts.count(s) == 0 ? nullptr : accounts.at(std::string{s});
  }

private:
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
  std::string name_;
};

class ObservableTransactionFactoryStub : public ObservableTransaction::Factory {
public:
  auto make() -> std::shared_ptr<ObservableTransaction> override { return {}; }
};

class BudgetObserverStub : public Budget::Observer {
public:
  auto newAccountName() -> std::string { return newAccountName_; }

  void notifyThatExpenseAccountHasBeenCreated(Account &account,
                                              std::string_view name) override {
    newAccountName_ = name;
    newAccount_ = &account;
  }

  void notifyThatCategoryAllocationHasChanged(std::string_view name,
                                              USD amount) override {
    categoryAllocations_[std::string{name}].push_back(amount);
  }

  void notifyThatUnallocatedIncomeHasChanged(USD amount) override {
    unallocatedIncome_.push_back(amount);
  }

  auto unallocatedIncome() -> std::vector<USD> { return unallocatedIncome_; }

  auto categoryAllocations(std::string_view s) -> std::vector<USD> {
    return categoryAllocations_.at(std::string{s});
  }

  auto newAccount() -> const Account * { return newAccount_; }

  void notifyThatNetIncomeHasChanged(USD b) override { netIncome_ = b; }

  auto netIncome() -> USD { return netIncome_; }

private:
  std::map<std::string, std::vector<USD>> categoryAllocations_;
  std::vector<USD> unallocatedIncome_;
  std::string newAccountName_;
  const Account *newAccount_{};
  USD netIncome_{};
};
} // namespace

static void add(AccountFactoryStub &factory, std::shared_ptr<Account> account,
                std::string_view accountName) {
  factory.add(std::move(account), accountName);
}

static auto addAccountStub(AccountFactoryStub &factory, std::string_view name)
    -> std::shared_ptr<AccountStub> {
  auto account{std::make_shared<AccountStub>()};
  add(factory, account, name);
  return account;
}

static void createAccount(Budget &budget, std::string_view name) {
  budget.createAccount(name);
}

static auto createAccountStub(Budget &budget, AccountFactoryStub &factory,
                              std::string_view name)
    -> std::shared_ptr<AccountStub> {
  auto account{addAccountStub(factory, name)};
  createAccount(budget, name);
  return account;
}

static void testBudgetInMemory(
    const std::function<void(AccountFactoryStub &factory,
                             AccountStub &incomeAccount, Budget &budget)> &f) {
  AccountFactoryStub factory;
  AccountStub incomeAccount;
  BudgetInMemory budget{incomeAccount, factory};
  f(factory, incomeAccount, budget);
}

static void addExpense(Budget &budget, std::string_view accountName,
                       const Transaction &t) {
  budget.addExpense(accountName, t);
}

static void addIncome(Budget &budget, const Transaction &t = {}) {
  budget.addIncome(t);
}

static void assertEqual(testcpplite::TestResult &result,
                        const SerializableAccount *expected,
                        const SerializableAccount *actual) {
  assertEqual(result, static_cast<const void *>(expected),
              static_cast<const void *>(actual));
}

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<SerializableAccount *> &expected,
                        const std::vector<SerializableAccount *> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (gsl::index i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<USD> &expected,
                        const std::vector<USD> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (gsl::index i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

static void assertAdded(testcpplite::TestResult &result, AccountStub &account,
                        const Transaction &t) {
  assertEqual(result, t, account.addedTransaction());
}

static void assertAdded(testcpplite::TestResult &result,
                        const std::shared_ptr<AccountStub> &account,
                        const Transaction &t) {
  assertAdded(result, *account, t);
}

static void assertRemoved(testcpplite::TestResult &result, AccountStub &account,
                          const Transaction &t) {
  assertEqual(result, t, account.removedTransaction());
}

static void assertRemoved(testcpplite::TestResult &result,
                          const std::shared_ptr<AccountStub> &account,
                          const Transaction &t) {
  assertRemoved(result, *account, t);
}

void addsIncomeToIncomeAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &, AccountStub &incomeAccount,
                               Budget &budget) {
    addIncome(budget,
              Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertAdded(result, incomeAccount,
                Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void addsExpenseToExpenseAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    addExpense(budget, "giraffe",
               Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    assertAdded(result, account,
                Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
  });
}

void addsExpenseToExistingAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    addExpense(budget, "giraffe",
               Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    add(factory, nullptr, "giraffe");
    addExpense(budget, "giraffe",
               Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertAdded(result, account,
                Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void transfersFromIncomeToExpenseAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto account{addAccountStub(factory, "giraffe")};
    budget.transferTo("giraffe", 456_cents);
    assertEqual(result, {-456_cents}, observer.unallocatedIncome());
    assertEqual(result, {456_cents}, observer.categoryAllocations("giraffe"));
  });
}

void savesAccounts(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    PersistentMemoryStub persistence;
    budget.save(persistence);
    assertEqual(result, &incomeAccount, persistence.primaryAccount());
    assertEqual(result, {giraffe.get(), leopard.get(), penguin.get()},
                persistence.secondaryAccounts());
  });
}

static void assertDeserializes(
    testcpplite::TestResult &result, BudgetDeserialization::Observer &observer,
    void (BudgetDeserialization::Observer::*notify)(AccountDeserialization &,
                                                    std::string_view),
    AccountDeserialization &deserialization, std::string_view name,
    const std::shared_ptr<AccountStub> &account) {
  (observer.*notify)(deserialization, name);
  assertEqual(result, &deserialization, account->deserialization());
}

void loadsAccounts(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto penguin{addAccountStub(factory, "penguin")};
    const auto leopard{addAccountStub(factory, "leopard")};

    PersistentMemoryStub persistence;
    budget.load(persistence);
    assertEqual(result, &budget, persistence.observer());

    AccountDeserializationStub deserialization;
    budget.notifyThatIncomeAccountIsReady(deserialization);
    assertEqual(result, &deserialization, incomeAccount.deserialization());
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatExpenseAccountIsReady,
        deserialization, "penguin", penguin);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatExpenseAccountIsReady,
        deserialization, "leopard", leopard);

    budget.save(persistence);
    assertEqual(result, &incomeAccount, persistence.primaryAccount());
    assertEqual(result, {leopard.get(), penguin.get()},
                persistence.secondaryAccounts());
  });
}

void clearsOldAccounts(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto penguin{addAccountStub(factory, "penguin")};
    const auto leopard{addAccountStub(factory, "leopard")};

    PersistentMemoryStub persistence;
    budget.load(persistence);

    AccountDeserializationStub deserialization;
    budget.notifyThatIncomeAccountIsReady(deserialization);
    budget.notifyThatExpenseAccountIsReady(deserialization, "penguin");
    budget.notifyThatExpenseAccountIsReady(deserialization, "leopard");

    const auto turtle{addAccountStub(factory, "turtle")};
    const auto tiger{addAccountStub(factory, "tiger")};

    budget.load(persistence);

    assertTrue(result, incomeAccount.cleared());
    assertTrue(result, penguin->removed());
    assertTrue(result, leopard->removed());

    budget.notifyThatIncomeAccountIsReady(deserialization);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatExpenseAccountIsReady,
        deserialization, "turtle", turtle);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatExpenseAccountIsReady,
        deserialization, "tiger", tiger);

    budget.save(persistence);
    assertEqual(result, &incomeAccount, persistence.primaryAccount());
    assertEqual(result, {tiger.get(), turtle.get()},
                persistence.secondaryAccounts());
  });
}

void removesExpenseFromAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    const auto account{createAccountStub(budget, factory, "giraffe")};
    budget.removeExpense("giraffe", Transaction{123_cents, "raccoon",
                                                Date{2013, Month::April, 3}});
    assertRemoved(
        result, account,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void doesNotRemoveExpenseFromNonexistentAccount(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    budget.removeExpense("giraffe", Transaction{123_cents, "raccoon",
                                                Date{2013, Month::April, 3}});
    assertFalse(result, account->transactionRemoved());
  });
}

void removesIncomeFromAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &, AccountStub &incomeAccount,
                               Budget &budget) {
    budget.removeIncome(
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertRemoved(
        result, incomeAccount,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void renamesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        budget.renameAccount("giraffe", "zebra");
        assertEqual(result, "zebra", giraffe->newName());
      });
}

void verifiesExpenseForExistingAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        budget.verifyExpense("giraffe",
                             {1_cents, "hi", Date{2020, Month::April, 1}});
        assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                    giraffe->verifiedTransaction());
      });
}

void verifiesIncome(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &, AccountStub &incomeAccount,
                               Budget &budget) {
    budget.verifyIncome({1_cents, "hi", Date{2020, Month::April, 1}});
    assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                incomeAccount.verifiedTransaction());
  });
}

void notifiesObserverOfDeserializedAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        BudgetObserverStub observer;
        budget.attach(&observer);
        const auto account{addAccountStub(factory, "giraffe")};
        AccountDeserializationStub deserialization;
        budget.notifyThatExpenseAccountIsReady(deserialization, "giraffe");
        assertEqual(result, "giraffe", observer.newAccountName());
        assertEqual(result, account.get(), observer.newAccount());
      });
}

static void assertReduced(testcpplite::TestResult &result,
                          AccountStub &account) {
  assertTrue(result, account.reduced());
}

void reducesEachAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    budget.reduce();
    assertReduced(result, incomeAccount);
    assertReduced(result, *giraffe);
    assertReduced(result, *penguin);
    assertReduced(result, *leopard);
  });
}

void notifiesThatNetIncomeHasChangedOnAddedIncome(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    incomeAccount.setBalance(456_cents);
    giraffe->setBalance(123_cents);
    penguin->setBalance(789_cents);
    leopard->setBalance(1111_cents);
    addIncome(budget);
    assertEqual(result, 456_cents + 123_cents + 789_cents + 1111_cents,
                observer.netIncome());
  });
}

void notifiesThatTotalBalanceHasChangedOnDebit(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    incomeAccount.setBalance(456_cents);
    giraffe->setBalance(123_cents);
    penguin->setBalance(789_cents);
    leopard->setBalance(1111_cents);
    addIncome(budget);
    assertEqual(result, 456_cents + 123_cents + 789_cents + 1111_cents,
                observer.netIncome());
  });
}

void notifiesThatTotalBalanceHasChangedOnRemoveAccount(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    incomeAccount.setBalance(456_cents);
    giraffe->setBalance(123_cents);
    penguin->setBalance(789_cents);
    leopard->setBalance(1111_cents);
    budget.removeAccount("penguin");
    assertEqual(result, 456_cents + 123_cents + 1111_cents,
                observer.netIncome());
  });
}

void removesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    budget.removeAccount("giraffe");
    assertTrue(result, giraffe->removed());
    PersistentMemoryStub persistence;
    budget.save(persistence);
    assertEqual(result, &incomeAccount, persistence.primaryAccount());
    assertEqual(result, {leopard.get(), penguin.get()},
                persistence.secondaryAccounts());
  });
}

void createsAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        createAccount(budget, "panda");
        assertEqual(result, "panda", factory.name());
      });
}

void doesNotOverwriteExistingAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe1{createAccountStub(budget, factory, "giraffe")};
        createAccount(budget, "giraffe");
        const auto giraffe2{createAccountStub(budget, factory, "giraffe")};
        createAccount(budget, "giraffe");
        PersistentMemoryStub persistence;
        budget.save(persistence);
        assertEqual(result, {giraffe1.get()}, persistence.secondaryAccounts());
      });
}

void closesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        BudgetObserverStub observer;
        budget.attach(&observer);
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        const auto penguin{createAccountStub(budget, factory, "penguin")};
        const auto leopard{createAccountStub(budget, factory, "leopard")};
        budget.allocate("giraffe", 7_cents);
        giraffe->setBalance(4_cents);
        budget.closeAccount("giraffe");
        assertEqual(result, {-7_cents, -4_cents}, observer.unallocatedIncome());
        assertEqual(result, {7_cents}, observer.categoryAllocations("giraffe"));
        PersistentMemoryStub persistence;
        budget.save(persistence);
        assertEqual(result, {leopard.get(), penguin.get()},
                    persistence.secondaryAccounts());
      });
}

void closesAccountHavingNegativeBalance(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    giraffe->setBalance(11_cents);
    budget.allocate("giraffe", 7_cents);
    budget.closeAccount("giraffe");
    assertEqual(result, {-7_cents, -11_cents}, observer.unallocatedIncome());
  });
}

void transfersAmountNeededToReachAllocation(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    budget.createAccount("giraffe");
    budget.allocate("giraffe", 456_cents);
    assertEqual(result, {456_cents}, observer.categoryAllocations("giraffe"));
    assertEqual(result, {-456_cents}, observer.unallocatedIncome());
  });
}

void transfersAmountFromAccountAllocatedSufficiently(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    budget.createAccount("giraffe");
    budget.allocate("giraffe", 101_cents);
    assertEqual(result, {101_cents}, observer.categoryAllocations("giraffe"));
    assertEqual(result, {-101_cents}, observer.unallocatedIncome());
  });
}

void restoresAccountsHavingNegativeBalances(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    budget.allocate("giraffe", 1_cents);
    budget.allocate("penguin", 5_cents);
    budget.allocate("leopard", 3_cents);
    giraffe->setBalance(6_cents);
    penguin->setBalance(2_cents);
    leopard->setBalance(4_cents);
    budget.restore();
    assertEqual(result, {1_cents, 6_cents},
                observer.categoryAllocations("giraffe"));
    assertEqual(result, {3_cents, 4_cents},
                observer.categoryAllocations("leopard"));
    assertEqual(result, {-1_cents, -6_cents, -9_cents, -14_cents, -15_cents},
                observer.unallocatedIncome());
  });
}
} // namespace sbash64::budget
