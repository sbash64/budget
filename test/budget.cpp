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

  void archiveVerifiedTransactions() override {
    archivedVerifiedTransactions_ = true;
  }

  [[nodiscard]] auto archivedVerifiedTransactions() const -> bool {
    return archivedVerifiedTransactions_;
  }

  auto balance() -> USD override { return balance_; }

  void remove() override { removed_ = true; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

  [[nodiscard]] auto cleared() const -> bool { return cleared_; }

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
  const AccountDeserialization *deserialization_{};
  USD balance_{};
  bool removed_{};
  bool cleared_{};
  bool archivedVerifiedTransactions_{};
};

class AccountFactoryStub : public Account::Factory {
public:
  void add(const std::shared_ptr<Account> &account, std::string_view) {
    accounts.push_back(account);
  }

  auto make() -> std::shared_ptr<Account> override {
    if (accounts.empty())
      return {};
    auto account{accounts.front()};
    accounts.erase(accounts.begin());
    return account;
  }

private:
  std::vector<std::shared_ptr<Account>> accounts;
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

static void add(AccountFactoryStub &factory,
                const std::shared_ptr<Account> &account,
                std::string_view accountName) {
  factory.add(account, accountName);
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

static void assertCategoryAllocations(testcpplite::TestResult &result,
                                      BudgetObserverStub &observer,
                                      const std::vector<USD> &expected,
                                      std::string_view name) {
  assertEqual(result, expected, observer.categoryAllocations(name));
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
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        BudgetObserverStub observer;
        budget.attach(&observer);
        addAccountStub(factory, "giraffe");
        budget.transferTo("giraffe", 456_cents);
        assertEqual(result, {-456_cents}, observer.unallocatedIncome());
        assertCategoryAllocations(result, observer, {456_cents}, "giraffe");
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
    assertEqual(result, &incomeAccount,
                persistence.incomeAccountWithFunds().account);
    assertEqual(result, giraffe.get(),
                persistence.expenseAccountsWithFunds().at(0).account);
    assertEqual(result, leopard.get(),
                persistence.expenseAccountsWithFunds().at(1).account);
    assertEqual(result, penguin.get(),
                persistence.expenseAccountsWithFunds().at(2).account);
  });
}

static void assertDeserializes(
    testcpplite::TestResult &result, BudgetDeserialization::Observer &observer,
    void (BudgetDeserialization::Observer::*notify)(AccountDeserialization &,
                                                    std::string_view, USD),
    AccountDeserialization &deserialization, std::string_view name,
    const std::shared_ptr<AccountStub> &account) {
  (observer.*notify)(deserialization, name, 1_cents);
  assertEqual(result, &deserialization, account->deserialization());
}

void loadsAccounts(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto penguin{addAccountStub(factory, "penguin")};
    const auto leopard{addAccountStub(factory, "leopard")};

    PersistentMemoryStub persistence;
    budget.load(persistence);
    assertEqual(result, &budget, persistence.observer());

    AccountDeserializationStub deserialization;
    budget.notifyThatIncomeAccountIsReady(deserialization, 1_cents);
    assertEqual(result, 1_cents, observer.unallocatedIncome().front());
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
    assertEqual(result, &incomeAccount,
                persistence.incomeAccountWithFunds().account);
    assertEqual(result, leopard.get(),
                persistence.expenseAccountsWithFunds().at(0).account);
    assertEqual(result, penguin.get(),
                persistence.expenseAccountsWithFunds().at(1).account);
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
    budget.notifyThatIncomeAccountIsReady(deserialization, 1_cents);
    budget.notifyThatExpenseAccountIsReady(deserialization, "penguin", 3_cents);
    budget.notifyThatExpenseAccountIsReady(deserialization, "leopard", 5_cents);

    const auto turtle{addAccountStub(factory, "turtle")};
    const auto tiger{addAccountStub(factory, "tiger")};

    budget.load(persistence);

    assertTrue(result, incomeAccount.cleared());
    assertTrue(result, penguin->removed());
    assertTrue(result, leopard->removed());

    budget.notifyThatIncomeAccountIsReady(deserialization, 2_cents);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatExpenseAccountIsReady,
        deserialization, "turtle", turtle);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatExpenseAccountIsReady,
        deserialization, "tiger", tiger);

    budget.save(persistence);
    assertEqual(result, &incomeAccount,
                persistence.incomeAccountWithFunds().account);
    assertEqual(result, tiger.get(),
                persistence.expenseAccountsWithFunds().at(0).account);
    assertEqual(result, turtle.get(),
                persistence.expenseAccountsWithFunds().at(1).account);
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
        BudgetObserverStub observer;
        budget.attach(&observer);
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        budget.allocate("giraffe", 3_cents);
        budget.renameAccount("giraffe", "zebra");
        budget.transferTo("zebra", 4_cents);
        assertCategoryAllocations(result, observer, {7_cents}, "zebra");
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
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto account{addAccountStub(factory, "giraffe")};
    AccountDeserializationStub deserialization;
    budget.notifyThatExpenseAccountIsReady(deserialization, "giraffe", 1_cents);
    assertEqual(result, "giraffe", observer.newAccountName());
    assertEqual(result, account.get(), observer.newAccount());
    assertEqual(result, 1_cents,
                observer.categoryAllocations("giraffe").front());
  });
}

static void assertReduced(testcpplite::TestResult &result,
                          AccountStub &account) {
  assertTrue(result, account.archivedVerifiedTransactions());
}

void reducesEachAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    incomeAccount.setBalance(4_cents);
    giraffe->setBalance(5_cents);
    penguin->setBalance(6_cents);
    leopard->setBalance(7_cents);
    budget.reduce();
    assertReduced(result, incomeAccount);
    assertReduced(result, *giraffe);
    assertReduced(result, *penguin);
    assertReduced(result, *leopard);
    assertEqual(result, {4_cents}, observer.unallocatedIncome());
    assertCategoryAllocations(result, observer, {-5_cents}, "giraffe");
    assertCategoryAllocations(result, observer, {-6_cents}, "penguin");
    assertCategoryAllocations(result, observer, {-7_cents}, "leopard");
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
    giraffe->setBalance(1_cents);
    penguin->setBalance(2_cents);
    leopard->setBalance(3_cents);
    incomeAccount.setBalance(4_cents);
    budget.reduce();
    giraffe->setBalance(5_cents);
    penguin->setBalance(6_cents);
    leopard->setBalance(7_cents);
    incomeAccount.setBalance(8_cents);
    addIncome(budget);
    assertEqual(result,
                8_cents + 4_cents - 5_cents - 6_cents - 7_cents - 1_cents -
                    2_cents - 3_cents,
                observer.netIncome());
  });
}

void notifiesThatNetIncomeHasChangedOnAddExpense(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    incomeAccount.setBalance(4_cents);
    giraffe->setBalance(5_cents);
    penguin->setBalance(6_cents);
    leopard->setBalance(7_cents);
    addExpense(budget, "penguin", {});
    assertEqual(result, 4_cents - 5_cents - 6_cents - 7_cents,
                observer.netIncome());
  });
}

void notifiesThatNetIncomeHasChangedOnRemoveAccount(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    incomeAccount.setBalance(4_cents);
    giraffe->setBalance(5_cents);
    penguin->setBalance(6_cents);
    leopard->setBalance(7_cents);
    budget.removeAccount("penguin");
    assertEqual(result, 4_cents - 5_cents - 7_cents, observer.netIncome());
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
    assertEqual(result, &incomeAccount,
                persistence.incomeAccountWithFunds().account);
    assertEqual(result, leopard.get(),
                persistence.expenseAccountsWithFunds().at(0).account);
    assertEqual(result, penguin.get(),
                persistence.expenseAccountsWithFunds().at(1).account);
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
        assertEqual(result, giraffe1.get(),
                    persistence.expenseAccountsWithFunds().at(0).account);
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
        assertCategoryAllocations(result, observer, {7_cents}, "giraffe");
        PersistentMemoryStub persistence;
        budget.save(persistence);
        assertEqual(result, leopard.get(),
                    persistence.expenseAccountsWithFunds().at(0).account);
        assertEqual(result, penguin.get(),
                    persistence.expenseAccountsWithFunds().at(1).account);
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
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        BudgetObserverStub observer;
        budget.attach(&observer);
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        budget.createAccount("giraffe");
        budget.allocate("giraffe", 456_cents);
        assertCategoryAllocations(result, observer, {456_cents}, "giraffe");
        assertEqual(result, {-456_cents}, observer.unallocatedIncome());
      });
}

void transfersAmountFromAccountAllocatedSufficiently(
    testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        BudgetObserverStub observer;
        budget.attach(&observer);
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        budget.createAccount("giraffe");
        budget.allocate("giraffe", 101_cents);
        assertCategoryAllocations(result, observer, {101_cents}, "giraffe");
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
