#include "budget.hpp"
#include "account-stub.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"

#include <sbash64/budget/budget.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

#include <gsl/gsl>

#include <functional>
#include <map>
#include <string_view>
#include <vector>

namespace sbash64::budget {
namespace {
class AccountDeserializationStub : public AccountDeserialization {
public:
  void load(Observer &) override {}
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

  auto newAccount() -> const Account * { return newAccount_; }

  void notifyThatNetIncomeHasChanged(USD b) override { netIncome_ = b; }

  auto netIncome() -> USD { return netIncome_; }

  void notifyThatHasBeenSaved() override { saved_ = true; }

  [[nodiscard]] auto saved() const -> bool { return saved_; }

  [[nodiscard]] auto hasUnsavedChanges() const -> bool {
    return hasUnsavedChanges_;
  }

  void notifyThatHasUnsavedChanges() override { hasUnsavedChanges_ = true; }

private:
  std::map<std::string, std::vector<USD>> categoryAllocations_;
  std::vector<USD> unallocatedIncome_;
  std::string newAccountName_;
  const Account *newAccount_{};
  USD netIncome_{};
  bool saved_{};
  bool hasUnsavedChanges_{};
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

static void
testBudgetInMemory(const std::function<
                   void(AccountFactoryStub &factory, AccountStub &incomeAccount,
                        BudgetObserverStub &observer, Budget &budget)> &f) {
  AccountFactoryStub factory;
  AccountStub incomeAccount;
  BudgetInMemory budget{incomeAccount, factory};
  BudgetObserverStub observer;
  budget.attach(observer);
  f(factory, incomeAccount, observer, budget);
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

static void assertHasUnsavedChanges(testcpplite::TestResult &result,
                                    BudgetObserverStub &observer) {
  assertTrue(result, observer.hasUnsavedChanges());
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

void notifiesThatHasUnsavedChangesWhenAddingIncome(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &, AccountStub &,
                               BudgetObserverStub &observer, Budget &budget) {
    addIncome(budget);
    assertHasUnsavedChanges(result, observer);
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

void notifiesThatHasUnsavedChangesWhenAddingExpense(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               BudgetObserverStub &observer, Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    addExpense(budget, "giraffe", Transaction{});
    assertHasUnsavedChanges(result, observer);
  });
}

void notifiesThatHasUnsavedChangesWhenCreatingAccount(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               BudgetObserverStub &observer, Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    createAccount(budget, "giraffe");
    assertHasUnsavedChanges(result, observer);
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
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{addAccountStub(factory, "giraffe")};
    budget.transferTo("giraffe", 456_cents);
    assertEqual(result, 456_cents, giraffe->increasedAllocationAmount());
    assertEqual(result, 456_cents, incomeAccount.decreasedAllocationAmount());
  });
}

void notifiesThatHasUnsavedChangesWhenTransferring(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               BudgetObserverStub &observer, Budget &budget) {
    const auto giraffe{addAccountStub(factory, "giraffe")};
    budget.transferTo("giraffe", 456_cents);
    assertHasUnsavedChanges(result, observer);
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
    assertEqual(result, &incomeAccount, persistence.incomeAccountWithFunds());
    assertEqual(result, giraffe.get(),
                persistence.expenseAccountsWithNames().at(0).account);
    assertEqual(result, leopard.get(),
                persistence.expenseAccountsWithNames().at(1).account);
    assertEqual(result, penguin.get(),
                persistence.expenseAccountsWithNames().at(2).account);
  });
}

void notifiesThatHasBeenSavedWhenSaved(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &, AccountStub &,
                               BudgetObserverStub &observer, Budget &budget) {
    PersistentMemoryStub persistence;
    budget.save(persistence);
    assertTrue(result, observer.saved());
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
    assertEqual(result, &incomeAccount, persistence.incomeAccountWithFunds());
    assertEqual(result, leopard.get(),
                persistence.expenseAccountsWithNames().at(0).account);
    assertEqual(result, penguin.get(),
                persistence.expenseAccountsWithNames().at(1).account);
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
    assertEqual(result, &incomeAccount, persistence.incomeAccountWithFunds());
    assertEqual(result, tiger.get(),
                persistence.expenseAccountsWithNames().at(0).account);
    assertEqual(result, turtle.get(),
                persistence.expenseAccountsWithNames().at(1).account);
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

void notifiesThatHasUnsavedChangesWhenRemovingExpense(
    testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto account{createAccountStub(budget, factory, "giraffe")};
        BudgetObserverStub observer;
        budget.attach(observer);
        budget.removeExpense("giraffe", Transaction{});
        assertHasUnsavedChanges(result, observer);
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

void notifiesThatHasUnsavedChangesWhenRemovingIncome(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &, AccountStub &,
                               BudgetObserverStub &observer, Budget &budget) {
    budget.removeIncome(Transaction{});
    assertHasUnsavedChanges(result, observer);
  });
}

void renamesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        BudgetObserverStub observer;
        budget.attach(observer);
        budget.renameAccount("giraffe", "zebra");
        assertEqual(result, "zebra", giraffe->newName);
        assertHasUnsavedChanges(result, observer);

        budget.transferTo("zebra", 4_cents);
        assertEqual(result, 4_cents, giraffe->increasedAllocationAmount());
      });
}

void ignoresRenamingNonexistentAccount(testcpplite::TestResult &) {
  testBudgetInMemory(
      [](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        budget.renameAccount("bear", "zebra");
      });
}

void ignoresRenameIfClobbersExisting(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        const auto zebra{createAccountStub(budget, factory, "zebra")};
        budget.renameAccount("zebra", "giraffe");
        assertFalse(result, zebra->renamed);
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

void ignoresVerificationOfNonexistentAccount(testcpplite::TestResult &) {
  testBudgetInMemory([](AccountFactoryStub &, AccountStub &, Budget &budget) {
    budget.verifyExpense("giraffe",
                         {1_cents, "hi", Date{2020, Month::April, 1}});
  });
}

void notifiesThatHasUnsavedChangesWhenVerifyingExpense(
    testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        BudgetObserverStub observer;
        budget.attach(observer);
        budget.verifyExpense("giraffe",
                             {1_cents, "hi", Date{2020, Month::April, 1}});
        assertHasUnsavedChanges(result, observer);
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

void notifiesThatHasUnsavedChangesWhenVerifyingIncome(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &, AccountStub &,
                               BudgetObserverStub &observer, Budget &budget) {
    budget.verifyIncome({1_cents, "hi", Date{2020, Month::April, 1}});
    assertHasUnsavedChanges(result, observer);
  });
}

void notifiesObserverOfDeserializedAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, AccountStub &,
                               BudgetObserverStub &observer, Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    AccountDeserializationStub deserialization;
    budget.notifyThatExpenseAccountIsReady(deserialization, "giraffe");
    assertEqual(result, "giraffe", observer.newAccountName());
    assertEqual(result, account.get(), observer.newAccount());
  });
}

static void assertAllocationIncreasedByResolvingVerifiedTransactions(
    testcpplite::TestResult &result, AccountStub &account) {
  assertTrue(result,
             account.increasedAllocationByResolvingVerifiedTransactions());
}

static void assertAllocationDecreasedByResolvingVerifiedTransactions(
    testcpplite::TestResult &result, AccountStub &account) {
  assertTrue(result,
             account.decreasedAllocationByResolvingVerifiedTransactions());
}

void reducesEachAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    budget.reduce();
    assertAllocationIncreasedByResolvingVerifiedTransactions(result,
                                                             incomeAccount);
    assertAllocationDecreasedByResolvingVerifiedTransactions(result, *giraffe);
    assertAllocationDecreasedByResolvingVerifiedTransactions(result, *penguin);
    assertAllocationDecreasedByResolvingVerifiedTransactions(result, *leopard);
  });
}

void notifiesThatHasUnsavedChangesWhenReducing(
    testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        const auto penguin{createAccountStub(budget, factory, "penguin")};
        const auto leopard{createAccountStub(budget, factory, "leopard")};
        BudgetObserverStub observer;
        budget.attach(observer);
        budget.reduce();
        assertHasUnsavedChanges(result, observer);
      });
}

void notifiesThatNetIncomeHasChangedOnAddedIncome(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount,
                               BudgetObserverStub &observer, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    giraffe->setBalance(5_cents);
    giraffe->setAllocated(9_cents);
    penguin->setBalance(6_cents);
    penguin->setAllocated(10_cents);
    leopard->setBalance(7_cents);
    leopard->setAllocated(11_cents);
    incomeAccount.setBalance(8_cents);
    incomeAccount.setAllocated(12_cents);
    addIncome(budget);
    assertEqual(result,
                8_cents - 5_cents - 6_cents - 7_cents + 9_cents + 10_cents +
                    11_cents + 12_cents,
                observer.netIncome());
  });
}

void notifiesThatNetIncomeHasChangedOnAddExpense(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount,
                               BudgetObserverStub &observer, Budget &budget) {
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
                               AccountStub &incomeAccount,
                               BudgetObserverStub &observer, Budget &budget) {
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
    assertEqual(result, &incomeAccount, persistence.incomeAccountWithFunds());
    assertEqual(result, leopard.get(),
                persistence.expenseAccountsWithNames().at(0).account);
    assertEqual(result, penguin.get(),
                persistence.expenseAccountsWithNames().at(1).account);
  });
}

void notifiesThatHasUnsavedChangesWhenRemovingAccount(
    testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        const auto penguin{createAccountStub(budget, factory, "penguin")};
        const auto leopard{createAccountStub(budget, factory, "leopard")};
        BudgetObserverStub observer;
        budget.attach(observer);
        budget.removeAccount("giraffe");
        assertHasUnsavedChanges(result, observer);
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
                    persistence.expenseAccountsWithNames().at(0).account);
      });
}

void closesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    giraffe->setBalance(4_cents);
    giraffe->setAllocated(5_cents);
    budget.closeAccount("giraffe");
    assertEqual(result, 1_cents, incomeAccount.increasedAllocationAmount());
    PersistentMemoryStub persistence;
    budget.save(persistence);
    assertEqual(result, leopard.get(),
                persistence.expenseAccountsWithNames().at(0).account);
    assertEqual(result, penguin.get(),
                persistence.expenseAccountsWithNames().at(1).account);
    assertEqual(result, "leopard",
                persistence.expenseAccountsWithNames().at(0).name);
    assertEqual(result, "penguin",
                persistence.expenseAccountsWithNames().at(1).name);
    assertEqual(result, std::vector<SerializableAccountWithName>::size_type{2},
                persistence.expenseAccountsWithNames().size());
  });
}

void notifiesThatHasUnsavedChangesWhenClosingAccount(
    testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        const auto penguin{createAccountStub(budget, factory, "penguin")};
        const auto leopard{createAccountStub(budget, factory, "leopard")};
        BudgetObserverStub observer;
        budget.attach(observer);
        budget.closeAccount("giraffe");
        assertHasUnsavedChanges(result, observer);
      });
}

void closesAccountHavingNegativeBalance(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    giraffe->setBalance(11_cents);
    giraffe->setAllocated(7_cents);
    budget.closeAccount("giraffe");
    assertEqual(result, 4_cents, incomeAccount.decreasedAllocationAmount());
  });
}

void transfersAmountNeededToReachAllocation(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    giraffe->setAllocated(4_cents);
    budget.allocate("giraffe", 7_cents);
    assertEqual(result, 3_cents, incomeAccount.decreasedAllocationAmount());
    assertEqual(result, 3_cents, giraffe->increasedAllocationAmount());
  });
}

void notifiesThatHasUnsavedChangesWhenAllocating(
    testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory, AccountStub &, Budget &budget) {
        const auto giraffe{createAccountStub(budget, factory, "giraffe")};
        BudgetObserverStub observer;
        budget.attach(observer);
        budget.allocate("giraffe", 7_cents);
        assertHasUnsavedChanges(result, observer);
      });
}

void transfersAmountFromAccountAllocatedSufficiently(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    giraffe->setAllocated(7_cents);
    budget.allocate("giraffe", 4_cents);
    assertEqual(result, 3_cents, incomeAccount.increasedAllocationAmount());
    assertEqual(result, 3_cents, giraffe->decreasedAllocationAmount());
  });
}

void restoresAccountsHavingNegativeBalances(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               AccountStub &incomeAccount, Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    giraffe->setAllocated(1_cents);
    giraffe->setBalance(6_cents);
    penguin->setAllocated(5_cents);
    penguin->setBalance(2_cents);
    leopard->setAllocated(3_cents);
    leopard->setBalance(4_cents);
    budget.restore();
    assertEqual(result, {6_cents - 1_cents, 4_cents - 3_cents},
                incomeAccount.decreasedAllocationAmounts());
    assertEqual(result, 6_cents - 1_cents,
                giraffe->increasedAllocationAmount());
    assertEqual(result, 4_cents - 3_cents,
                leopard->increasedAllocationAmount());
  });
}
} // namespace sbash64::budget
