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

  auto withdrawn() -> USD { return withdrawn_; }

  void withdraw(USD usd) override {
    withdrawals_.push_back(usd);
    withdrawn_ = usd;
  }

  auto deposited() -> USD { return deposited_; }

  void deposit(USD usd) override { deposited_ = usd; }

  auto withdrawals() -> std::vector<USD> { return withdrawals_; }

private:
  std::vector<USD> withdrawals_;
  std::string newName_;
  const AccountDeserialization *deserialization_{};
  USD balance_{};
  USD withdrawn_{};
  USD deposited_{};
  bool removed_{};
  bool cleared_{};
  bool reduced_{};
};

class IncomeAccountStub : public AccountStub, public IncomeAccount {
public:
  void add(const Transaction &t) override { creditedTransaction_ = t; }

  auto creditedTransaction() -> Transaction { return creditedTransaction_; }

  auto removedCredit() -> Transaction { return removedCredit_; }

  void remove(const Transaction &t) override { removedCredit_ = t; }

  auto creditToVerify() -> Transaction { return creditToVerify_; }

  void verify(const Transaction &t) override { creditToVerify_ = t; }

  void notifyThatIsReady(TransactionDeserialization &) override {}

private:
  Transaction creditedTransaction_;
  Transaction removedCredit_;
  Transaction creditToVerify_;
};

class ExpenseAccountStub : public AccountStub, public ExpenseAccount {
public:
  void add(const Transaction &t) override { debitedTransaction_ = t; }

  void remove(const Transaction &t) override {
    debitRemoved_ = true;
    removedDebit_ = t;
  }

  [[nodiscard]] auto debitRemoved() const -> bool { return debitRemoved_; }

  auto debitToVerify() -> Transaction { return debitToVerify_; }

  void verify(const Transaction &t) override { debitToVerify_ = t; }

  auto debitedTransaction() -> Transaction { return debitedTransaction_; }

  auto removedDebit() -> Transaction { return removedDebit_; }

  void notifyThatIsReady(TransactionDeserialization &) override {}

private:
  Transaction debitToVerify_;
  Transaction debitedTransaction_;
  Transaction removedDebit_;
  bool debitRemoved_{};
};

class AccountFactoryStub : public ExpenseAccount::Factory {
public:
  void add(std::shared_ptr<ExpenseAccount> account, std::string_view name) {
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

  void notifyThatNewAccountHasBeenCreated(Account &account,
                                          std::string_view name) override {
    newAccountName_ = name;
    newAccount_ = &account;
  }

  auto newAccount() -> const Account * { return newAccount_; }

  void notifyThatTotalBalanceHasChanged(USD b) override { totalBalance_ = b; }

  auto totalBalance() -> USD { return totalBalance_; }

private:
  std::string newAccountName_;
  const Account *newAccount_{};
  USD totalBalance_{};
};
} // namespace

static void add(AccountFactoryStub &factory,
                std::shared_ptr<ExpenseAccount> account,
                std::string_view accountName) {
  factory.add(std::move(account), accountName);
}

static auto addAccountStub(AccountFactoryStub &factory, std::string_view name)
    -> std::shared_ptr<ExpenseAccountStub> {
  auto account{std::make_shared<ExpenseAccountStub>()};
  add(factory, account, name);
  return account;
}

static void createAccount(Budget &budget, std::string_view name) {
  budget.createAccount(name);
}

static auto createAccountStub(Budget &budget, AccountFactoryStub &factory,
                              std::string_view name)
    -> std::shared_ptr<ExpenseAccountStub> {
  auto account{addAccountStub(factory, name)};
  createAccount(budget, name);
  return account;
}

static void testBudgetInMemory(
    const std::function<void(AccountFactoryStub &factory,
                             IncomeAccountStub &masterAccount, Budget &budget)>
        &f) {
  AccountFactoryStub factory;
  IncomeAccountStub masterAccount;
  BudgetInMemory budget{masterAccount, factory};
  f(factory, masterAccount, budget);
}

static void debit(Budget &budget, std::string_view accountName,
                  const Transaction &t) {
  budget.debit(accountName, t);
}

static void credit(Budget &budget, const Transaction &t = {}) {
  budget.credit(t);
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

static void assertDebited(testcpplite::TestResult &result,
                          const std::shared_ptr<ExpenseAccountStub> &account,
                          const Transaction &t) {
  assertEqual(result, t, account->debitedTransaction());
}

static void assertCredited(testcpplite::TestResult &result,
                           IncomeAccountStub &account, const Transaction &t) {
  assertEqual(result, t, account.creditedTransaction());
}

static void assertCreditRemoved(testcpplite::TestResult &result,
                                IncomeAccountStub &account,
                                const Transaction &t) {
  assertEqual(result, t, account.removedCredit());
}

static void
assertDebitRemoved(testcpplite::TestResult &result,
                   const std::shared_ptr<ExpenseAccountStub> &account,
                   const Transaction &t) {
  assertEqual(result, t, account->removedDebit());
}

void creditsMasterAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    credit(budget,
           Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertCredited(
        result, masterAccount,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void debitsNonexistentAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    debit(budget, "giraffe",
          Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    assertDebited(
        result, account,
        Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
  });
}

void debitsExistingAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    debit(budget, "giraffe",
          Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    add(factory, nullptr, "giraffe");
    debit(budget, "giraffe",
          Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertDebited(
        result, account,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void transfersFromMasterAccountToOther(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    budget.transferTo("giraffe", 456_cents);
    assertEqual(result, 456_cents, masterAccount.withdrawn());
    assertEqual(result, 456_cents, account->deposited());
  });
}

void savesAccounts(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    PersistentMemoryStub persistence;
    budget.save(persistence);
    assertEqual(result, &masterAccount, persistence.primaryAccount());
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
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto penguin{addAccountStub(factory, "penguin")};
    const auto leopard{addAccountStub(factory, "leopard")};

    PersistentMemoryStub persistence;
    budget.load(persistence);
    assertEqual(result, &budget, persistence.observer());

    AccountDeserializationStub deserialization;
    budget.notifyThatPrimaryAccountIsReady(deserialization, "");
    assertEqual(result, &deserialization, masterAccount.deserialization());
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatSecondaryAccountIsReady,
        deserialization, "penguin", penguin);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatSecondaryAccountIsReady,
        deserialization, "leopard", leopard);

    budget.save(persistence);
    assertEqual(result, &masterAccount, persistence.primaryAccount());
    assertEqual(result, {leopard.get(), penguin.get()},
                persistence.secondaryAccounts());
  });
}

void clearsOldAccounts(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto penguin{addAccountStub(factory, "penguin")};
    const auto leopard{addAccountStub(factory, "leopard")};

    PersistentMemoryStub persistence;
    budget.load(persistence);

    AccountDeserializationStub deserialization;
    budget.notifyThatPrimaryAccountIsReady(deserialization, "");
    budget.notifyThatSecondaryAccountIsReady(deserialization, "penguin");
    budget.notifyThatSecondaryAccountIsReady(deserialization, "leopard");

    const auto turtle{addAccountStub(factory, "turtle")};
    const auto tiger{addAccountStub(factory, "tiger")};

    budget.load(persistence);

    assertTrue(result, masterAccount.cleared());
    assertTrue(result, penguin->removed());
    assertTrue(result, leopard->removed());

    budget.notifyThatPrimaryAccountIsReady(deserialization, "");
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatSecondaryAccountIsReady,
        deserialization, "turtle", turtle);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatSecondaryAccountIsReady,
        deserialization, "tiger", tiger);

    budget.save(persistence);
    assertEqual(result, &masterAccount, persistence.primaryAccount());
    assertEqual(result, {tiger.get(), turtle.get()},
                persistence.secondaryAccounts());
  });
}

void removesDebit(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
    const auto account{createAccountStub(budget, factory, "giraffe")};
    budget.removeDebit("giraffe", Transaction{123_cents, "raccoon",
                                              Date{2013, Month::April, 3}});
    assertDebitRemoved(
        result, account,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void doesNotRemoveDebitFromNonexistentAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    budget.removeDebit("giraffe", Transaction{123_cents, "raccoon",
                                              Date{2013, Month::April, 3}});
    assertFalse(result, account->debitRemoved());
  });
}

void removesCredit(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    budget.removeCredit(
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertCreditRemoved(
        result, masterAccount,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void renamesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    budget.renameAccount("giraffe", "zebra");
    assertEqual(result, "zebra", giraffe->newName());
  });
}

void verifiesDebitForExistingAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    budget.verifyDebit("giraffe", {1_cents, "hi", Date{2020, Month::April, 1}});
    assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                giraffe->debitToVerify());
  });
}

void verifiesCreditForMasterAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    budget.verifyCredit({1_cents, "hi", Date{2020, Month::April, 1}});
    assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                masterAccount.creditToVerify());
  });
}

void notifiesObserverOfDeserializedAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto account{addAccountStub(factory, "giraffe")};
    AccountDeserializationStub deserialization;
    budget.notifyThatSecondaryAccountIsReady(deserialization, "giraffe");
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
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    budget.reduce();
    assertReduced(result, masterAccount);
    assertReduced(result, *giraffe);
    assertReduced(result, *penguin);
    assertReduced(result, *leopard);
  });
}

void notifiesThatTotalBalanceHasChangedOnCredit(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    masterAccount.setBalance(456_cents);
    giraffe->setBalance(123_cents);
    penguin->setBalance(789_cents);
    leopard->setBalance(1111_cents);
    credit(budget);
    assertEqual(result, 456_cents + 123_cents + 789_cents + 1111_cents,
                observer.totalBalance());
  });
}

void notifiesThatTotalBalanceHasChangedOnDebit(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    masterAccount.setBalance(456_cents);
    giraffe->setBalance(123_cents);
    penguin->setBalance(789_cents);
    leopard->setBalance(1111_cents);
    credit(budget);
    assertEqual(result, 456_cents + 123_cents + 789_cents + 1111_cents,
                observer.totalBalance());
  });
}

void notifiesThatTotalBalanceHasChangedOnRemoveAccount(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    BudgetObserverStub observer;
    budget.attach(&observer);
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    masterAccount.setBalance(456_cents);
    giraffe->setBalance(123_cents);
    penguin->setBalance(789_cents);
    leopard->setBalance(1111_cents);
    budget.removeAccount("penguin");
    assertEqual(result, 456_cents + 123_cents + 1111_cents,
                observer.totalBalance());
  });
}

void removesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    budget.removeAccount("giraffe");
    assertTrue(result, giraffe->removed());
    PersistentMemoryStub persistence;
    budget.save(persistence);
    assertEqual(result, &masterAccount, persistence.primaryAccount());
    assertEqual(result, {leopard.get(), penguin.get()},
                persistence.secondaryAccounts());
  });
}

void createsAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
    createAccount(budget, "panda");
    assertEqual(result, "panda", factory.name());
  });
}

void doesNotOverwriteExistingAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory, IncomeAccountStub &,
                               Budget &budget) {
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
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    giraffe->setBalance(123_cents);
    budget.closeAccount("giraffe");
    PersistentMemoryStub persistence;
    budget.save(persistence);
    assertEqual(result, {leopard.get(), penguin.get()},
                persistence.secondaryAccounts());
    assertEqual(result, 123_cents, masterAccount.deposited());
  });
}

void closesAccountHavingNegativeBalance(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    giraffe->setBalance(-123_cents);
    budget.closeAccount("giraffe");
    assertEqual(result, 123_cents, masterAccount.withdrawn());
  });
}

void transfersAmountNeededToReachAllocation(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    giraffe->setBalance(-123_cents);
    budget.createAccount("giraffe");
    budget.allocate("giraffe", 456_cents);
    assertEqual(result, 579_cents, giraffe->deposited());
    assertEqual(result, 579_cents, masterAccount.withdrawn());
  });
}

void transfersAmountFromAccountAllocatedSufficiently(
    testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    giraffe->setBalance(123_cents);
    budget.createAccount("giraffe");
    budget.allocate("giraffe", 101_cents);
    assertEqual(result, 22_cents, giraffe->withdrawn());
    assertEqual(result, 22_cents, masterAccount.deposited());
  });
}

void restoresAccountsHavingNegativeBalances(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               IncomeAccountStub &masterAccount,
                               Budget &budget) {
    const auto giraffe{createAccountStub(budget, factory, "giraffe")};
    const auto penguin{createAccountStub(budget, factory, "penguin")};
    const auto leopard{createAccountStub(budget, factory, "leopard")};
    giraffe->setBalance(-12_cents);
    penguin->setBalance(34_cents);
    leopard->setBalance(-56_cents);
    budget.createAccount("giraffe");
    budget.createAccount("penguin");
    budget.createAccount("leopard");
    budget.restore();
    assertEqual(result, 12_cents, giraffe->deposited());
    assertEqual(result, 56_cents, leopard->deposited());
    assertEqual(result, {12_cents, 56_cents}, masterAccount.withdrawals());
  });
}
} // namespace sbash64::budget
