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

class AccountStub : public Account {
public:
  void setBalance(USD b) { balance_ = b; }

  void attach(Observer *) override {}

  auto creditedTransaction() -> Transaction { return creditedTransaction_; }

  void credit(const Transaction &t) override { creditedTransaction_ = t; }

  auto debitedTransaction() -> Transaction { return debitedTransaction_; }

  void debit(const Transaction &t) override { debitedTransaction_ = t; }

  auto removedDebit() -> Transaction { return removedDebit_; }

  void removeDebit(const Transaction &t) override {
    debitRemoved_ = true;
    removedDebit_ = t;
  }

  auto removedCredit() -> Transaction { return removedCredit_; }

  void removeCredit(const Transaction &t) override { removedCredit_ = t; }

  void save(AccountSerialization &) override {}

  void load(AccountDeserialization &a) override { deserialization_ = &a; }

  auto deserialization() -> const AccountDeserialization * {
    return deserialization_;
  }

  auto newName() -> std::string { return newName_; }

  void rename(std::string_view s) override { newName_ = s; }

  auto debitToVerify() -> Transaction { return debitToVerify_; }

  void verifyDebit(const Transaction &t) override { debitToVerify_ = t; }

  auto creditToVerify() -> Transaction { return creditToVerify_; }

  void verifyCredit(const Transaction &t) override { creditToVerify_ = t; }

  void notifyThatCreditIsReady(TransactionDeserialization &) override {}

  void notifyThatDebitIsReady(TransactionDeserialization &) override {}

  auto reducedDate() -> Date { return reducedDate_; }

  void reduce(const Date &date) override { reducedDate_ = date; }

  auto balance() -> USD override { return balance_; }

  void remove() override { removed_ = true; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

  [[nodiscard]] auto debitRemoved() const -> bool { return debitRemoved_; }

private:
  Transaction creditToVerify_;
  Transaction debitToVerify_;
  Transaction creditedTransaction_;
  Transaction debitedTransaction_;
  Transaction removedDebit_;
  Transaction removedCredit_;
  std::vector<Transaction> foundUnverifiedDebits;
  std::vector<Transaction> foundUnverifiedCredits;
  Date reducedDate_;
  std::string newName_;
  const AccountDeserialization *deserialization_{};
  USD balance_{};
  bool removed_{};
  bool debitRemoved_{};
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

static void testBudgetInMemory(
    const std::function<void(AccountFactoryStub &factory,
                             const std::shared_ptr<AccountStub> &masterAccount,
                             Budget &budget)> &f) {
  AccountFactoryStub factory;
  const auto masterAccount{addAccountStub(factory, masterAccountName.data())};
  BudgetInMemory budget{factory};
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

static void assertDebited(testcpplite::TestResult &result,
                          const std::shared_ptr<AccountStub> &account,
                          const Transaction &t) {
  assertEqual(result, t, account->debitedTransaction());
}

static void assertCredited(testcpplite::TestResult &result,
                           const std::shared_ptr<AccountStub> &account,
                           const Transaction &t) {
  assertEqual(result, t, account->creditedTransaction());
}

static void assertCreditRemoved(testcpplite::TestResult &result,
                                const std::shared_ptr<AccountStub> &account,
                                const Transaction &t) {
  assertEqual(result, t, account->removedCredit());
}

static void assertDebitRemoved(testcpplite::TestResult &result,
                               const std::shared_ptr<AccountStub> &account,
                               const Transaction &t) {
  assertEqual(result, t, account->removedDebit());
}

void createsMasterAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &, Budget &) {
    assertEqual(result, masterAccountName.data(), factory.name());
  });
}

void creditsMasterAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        credit(budget,
               Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
        assertCredited(
            result, masterAccount,
            Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
      });
}

void debitsNonexistentAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &,
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
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &,
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
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        const auto account{addAccountStub(factory, "giraffe")};
        budget.transferTo("giraffe", 456_cents, Date{1776, Month::July, 4});
        assertCredited(result, account,
                       Transaction{456_cents, "transfer from master",
                                   Date{1776, Month::July, 4}});
        assertDebited(result, masterAccount,
                      Transaction{456_cents, "transfer to giraffe",
                                  Date{1776, Month::July, 4}});
      });
}

void savesAccounts(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        const auto giraffe{addAccountStub(factory, "giraffe")};
        const auto penguin{addAccountStub(factory, "penguin")};
        const auto leopard{addAccountStub(factory, "leopard")};
        debit(budget, "giraffe", {});
        debit(budget, "penguin", {});
        debit(budget, "leopard", {});
        PersistentMemoryStub persistence;
        budget.save(persistence);
        assertEqual(result, masterAccount.get(), persistence.primaryAccount());
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
                               const std::shared_ptr<AccountStub> &,
                               Budget &budget) {
    const auto giraffe{addAccountStub(factory, "giraffe")};
    const auto penguin{addAccountStub(factory, "penguin")};
    const auto leopard{addAccountStub(factory, "leopard")};

    PersistentMemoryStub persistence;
    budget.load(persistence);
    assertEqual(result, &budget, persistence.observer());

    AccountDeserializationStub deserialization;
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatPrimaryAccountIsReady,
        deserialization, "giraffe", giraffe);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatSecondaryAccountIsReady,
        deserialization, "penguin", penguin);
    assertDeserializes(
        result, budget,
        &BudgetDeserialization::Observer::notifyThatSecondaryAccountIsReady,
        deserialization, "leopard", leopard);

    budget.save(persistence);
    assertEqual(result, giraffe.get(), persistence.primaryAccount());
    assertEqual(result, {leopard.get(), penguin.get()},
                persistence.secondaryAccounts());
  });
}

void removesDebit(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    debit(budget, "giraffe", {});
    budget.removeDebit("giraffe", Transaction{123_cents, "raccoon",
                                              Date{2013, Month::April, 3}});
    assertDebitRemoved(
        result, account,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void doesNotRemoveDebitFromNonexistentAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &,
                               Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    budget.removeDebit("giraffe", Transaction{123_cents, "raccoon",
                                              Date{2013, Month::April, 3}});
    assertFalse(result, account->debitRemoved());
  });
}

void removesCredit(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        budget.removeCredit(
            Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
        assertCreditRemoved(
            result, masterAccount,
            Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
      });
}

void removesTransfer(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        const auto account{addAccountStub(factory, "giraffe")};
        debit(budget, "giraffe", {});
        budget.removeTransfer("giraffe", 456_cents, Date{1776, Month::July, 4});
        assertCreditRemoved(result, account,
                            Transaction{456_cents, "transfer from master",
                                        Date{1776, Month::July, 4}});
        assertDebitRemoved(result, masterAccount,
                           Transaction{456_cents, "transfer to giraffe",
                                       Date{1776, Month::July, 4}});
      });
}

void renamesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &,
                               Budget &budget) {
    const auto giraffe{addAccountStub(factory, "giraffe")};
    debit(budget, "giraffe", {});
    budget.renameAccount("giraffe", "zebra");
    assertEqual(result, "zebra", giraffe->newName());
  });
}

void verifiesDebitForExistingAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &,
                               Budget &budget) {
    const auto giraffe{addAccountStub(factory, "giraffe")};
    debit(budget, "giraffe", {});
    budget.verifyDebit("giraffe", {1_cents, "hi", Date{2020, Month::April, 1}});
    assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                giraffe->debitToVerify());
  });
}

void verifiesCreditForMasterAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        budget.verifyCredit({1_cents, "hi", Date{2020, Month::April, 1}});
        assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                    masterAccount->creditToVerify());
      });
}

void verifiesTransferTransactions(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](
                         AccountFactoryStub &factory,
                         const std::shared_ptr<AccountStub> &masterAccount,
                         Budget &budget) {
    const auto account{addAccountStub(factory, "giraffe")};
    budget.transferTo("giraffe", 456_cents, Date{1776, Month::July, 4});
    assertEqual(result,
                {456_cents, "transfer from master", Date{1776, Month::July, 4}},
                account->creditToVerify());
    assertEqual(result,
                {456_cents, "transfer to giraffe", Date{1776, Month::July, 4}},
                masterAccount->debitToVerify());
  });
}

void notifiesObserverOfDeserializedAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &,
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

static void assertReduced(testcpplite::TestResult &result, const Date &date,
                          const std::shared_ptr<AccountStub> &account) {
  assertEqual(result, date, account->reducedDate());
}

void reducesEachAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        const auto giraffe{addAccountStub(factory, "giraffe")};
        const auto penguin{addAccountStub(factory, "penguin")};
        const auto leopard{addAccountStub(factory, "leopard")};
        debit(budget, "giraffe", {});
        debit(budget, "penguin", {});
        debit(budget, "leopard", {});
        budget.reduce(Date{2021, Month::March, 13});
        assertReduced(result, Date{2021, Month::March, 13}, masterAccount);
        assertReduced(result, Date{2021, Month::March, 13}, giraffe);
        assertReduced(result, Date{2021, Month::March, 13}, penguin);
        assertReduced(result, Date{2021, Month::March, 13}, leopard);
      });
}

void notifiesThatTotalBalanceHasChangedOnCredit(
    testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        BudgetObserverStub observer;
        budget.attach(&observer);
        const auto giraffe{addAccountStub(factory, "giraffe")};
        const auto penguin{addAccountStub(factory, "penguin")};
        const auto leopard{addAccountStub(factory, "leopard")};
        debit(budget, "giraffe", {});
        debit(budget, "penguin", {});
        debit(budget, "leopard", {});
        masterAccount->setBalance(456_cents);
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
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        BudgetObserverStub observer;
        budget.attach(&observer);
        const auto giraffe{addAccountStub(factory, "giraffe")};
        const auto penguin{addAccountStub(factory, "penguin")};
        const auto leopard{addAccountStub(factory, "leopard")};
        debit(budget, "giraffe", {});
        debit(budget, "penguin", {});
        debit(budget, "leopard", {});
        masterAccount->setBalance(456_cents);
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
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        BudgetObserverStub observer;
        budget.attach(&observer);
        const auto giraffe{addAccountStub(factory, "giraffe")};
        const auto penguin{addAccountStub(factory, "penguin")};
        const auto leopard{addAccountStub(factory, "leopard")};
        debit(budget, "giraffe", {});
        debit(budget, "penguin", {});
        debit(budget, "leopard", {});
        masterAccount->setBalance(456_cents);
        giraffe->setBalance(123_cents);
        penguin->setBalance(789_cents);
        leopard->setBalance(1111_cents);
        budget.removeAccount("penguin");
        assertEqual(result, 456_cents + 123_cents + 1111_cents,
                    observer.totalBalance());
      });
}

void removesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        const auto giraffe{addAccountStub(factory, "giraffe")};
        const auto penguin{addAccountStub(factory, "penguin")};
        const auto leopard{addAccountStub(factory, "leopard")};
        debit(budget, "giraffe", {});
        debit(budget, "penguin", {});
        debit(budget, "leopard", {});
        budget.removeAccount("giraffe");
        assertTrue(result, giraffe->removed());
        PersistentMemoryStub persistence;
        budget.save(persistence);
        assertEqual(result, masterAccount.get(), persistence.primaryAccount());
        assertEqual(result, {leopard.get(), penguin.get()},
                    persistence.secondaryAccounts());
      });
}

void createsAccount(testcpplite::TestResult &result) {
  testBudgetInMemory([&result](AccountFactoryStub &factory,
                               const std::shared_ptr<AccountStub> &,
                               Budget &budget) {
    budget.createAccount("panda");
    assertEqual(result, "panda", factory.name());
  });
}

void closesAccount(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        const auto giraffe{addAccountStub(factory, "giraffe")};
        const auto penguin{addAccountStub(factory, "penguin")};
        const auto leopard{addAccountStub(factory, "leopard")};
        budget.createAccount("giraffe");
        budget.createAccount("penguin");
        budget.createAccount("leopard");
        giraffe->setBalance(123_cents);
        budget.closeAccount("giraffe", Date{2021, Month::April, 3});
        PersistentMemoryStub persistence;
        budget.save(persistence);
        assertEqual(result, {leopard.get(), penguin.get()},
                    persistence.secondaryAccounts());
        assertEqual(result,
                    {123_cents, "close giraffe", Date{2021, Month::April, 3}},
                    masterAccount->creditedTransaction());
        assertEqual(result,
                    {123_cents, "close giraffe", Date{2021, Month::April, 3}},
                    masterAccount->creditToVerify());
      });
}

void closesAccountHavingNegativeBalance(testcpplite::TestResult &result) {
  testBudgetInMemory(
      [&result](AccountFactoryStub &factory,
                const std::shared_ptr<AccountStub> &masterAccount,
                Budget &budget) {
        const auto giraffe{addAccountStub(factory, "giraffe")};
        debit(budget, "giraffe", {});
        giraffe->setBalance(-123_cents);
        budget.closeAccount("giraffe", Date{2021, Month::April, 3});
        assertEqual(result,
                    {123_cents, "close giraffe", Date{2021, Month::April, 3}},
                    masterAccount->debitedTransaction());
        assertEqual(result,
                    {123_cents, "close giraffe", Date{2021, Month::April, 3}},
                    masterAccount->debitToVerify());
      });
}
} // namespace sbash64::budget
