#include "bank.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"

#include <sbash64/budget/bank.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

#include <gsl/gsl>

#include <functional>
#include <map>
#include <utility>

namespace sbash64::budget::bank {
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

  void removeDebit(const Transaction &t) override { removedDebit_ = t; }

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

  void notifyThatCreditIsReady(TransactionRecordDeserialization &) override {}

  void notifyThatDebitIsReady(TransactionRecordDeserialization &) override {}

  auto reducedDate() -> Date { return reducedDate_; }

  void reduce(const Date &date) override { reducedDate_ = date; }

  auto balance() -> USD override { return balance_; }

  void remove() override { removed_ = true; }

  auto removed() -> bool { return removed_; }

private:
  Transaction creditToVerify_;
  Transaction debitToVerify_;
  Transaction creditedTransaction_;
  Transaction debitedTransaction_;
  Transaction removedDebit_;
  Transaction removedCredit_;
  Transactions foundUnverifiedDebits;
  Transactions foundUnverifiedCredits;
  Date reducedDate_;
  std::string newName_;
  const AccountDeserialization *deserialization_{};
  USD balance_{};
  bool removed_{};
};

class AccountFactoryStub : public Account::Factory {
public:
  void add(std::shared_ptr<Account> account, std::string_view name) {
    accounts[std::string{name}] = std::move(account);
  }

  auto name() -> std::string { return name_; }

  auto make(std::string_view s, TransactionRecord::Factory &)
      -> std::shared_ptr<Account> override {
    name_ = s;
    return accounts.count(s) == 0 ? nullptr : accounts.at(std::string{s});
  }

private:
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
  std::string name_;
};

class TransactionRecordFactoryStub : public TransactionRecord::Factory {
public:
  auto make() -> std::shared_ptr<TransactionRecord> override { return {}; }
};

class BankObserverStub : public BudgetInMemory::Observer {
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

  auto removedAccountName() -> std::string { return removedAccountName_; }

private:
  std::string newAccountName_;
  std::string removedAccountName_;
  const Account *newAccount_{};
  USD totalBalance_{};
};
} // namespace

static void add(AccountFactoryStub &factory, std::shared_ptr<Account> account,
                std::string_view accountName) {
  factory.add(std::move(account), accountName);
}

static void testBank(
    const std::function<void(AccountFactoryStub &factory,
                             const std::shared_ptr<AccountStub> &masterAccount,
                             BudgetInMemory &bank)> &f) {
  AccountFactoryStub factory;
  const auto masterAccount{std::make_shared<AccountStub>()};
  add(factory, masterAccount, masterAccountName.data());
  TransactionRecordFactoryStub transactionRecordFactory;
  BudgetInMemory bank{factory, transactionRecordFactory};
  f(factory, masterAccount, bank);
}

static void debit(BudgetInMemory &bank, std::string_view accountName,
                  const Transaction &t) {
  bank.debit(accountName, t);
}

static void credit(BudgetInMemory &bank, const Transaction &t = {}) {
  bank.credit(t);
}

static void assertEqual(testcpplite::TestResult &result,
                        const std::vector<Account *> &expected,
                        const std::vector<Account *> &actual) {
  assertEqual(result, expected.size(), actual.size());
  for (gsl::index i{0}; i < expected.size(); ++i)
    assertEqual(result, expected.at(i), actual.at(i));
}

static void assertContains(testcpplite::TestResult &result,
                           const std::vector<Account *> &accounts,
                           const std::shared_ptr<Account> &account) {
  assertTrue(result, find(accounts.begin(), accounts.end(), account.get()) !=
                         accounts.end());
}

static void
assertContainsSecondaryAccount(testcpplite::TestResult &result,
                               PersistentMemoryStub &persistentMemory,
                               const std::shared_ptr<Account> &account) {
  assertContains(result, persistentMemory.secondaryAccounts(), account);
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

void createsMasterAccountOnConstruction(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &) {
    assertEqual(result, masterAccountName.data(), factory.name());
  });
}

void creditsMasterAccountWhenCredited(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    credit(bank,
           Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertCredited(
        result, masterAccount,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void createsAccountWhenDebitingNonexistent(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    debit(bank, "giraffe",
          Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    assertDebited(
        result, account,
        Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
  });
}

void debitsExistingAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    debit(bank, "giraffe",
          Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    add(factory, nullptr, "giraffe");
    debit(bank, "giraffe",
          Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertDebited(
        result, account,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void transferDebitsMasterAndCreditsOther(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    bank.transferTo("giraffe", 456_cents, Date{1776, Month::July, 4});
    assertCredited(result, account,
                   Transaction{456_cents, "transfer from master",
                               Date{1776, Month::July, 4}});
    assertDebited(result, masterAccount,
                  Transaction{456_cents, "transfer to giraffe",
                              Date{1776, Month::July, 4}});
  });
}

void showShowsAccountsInAlphabeticOrder(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    debit(bank, "giraffe", {});
    debit(bank, "penguin", {});
    debit(bank, "leopard", {});
    PersistentMemoryStub persistent;
    bank.save(persistent);
    assertEqual(result, masterAccount.get(), persistent.primaryAccount());
    assertEqual(result, giraffe.get(), persistent.secondaryAccounts().at(0));
    assertEqual(result, leopard.get(), persistent.secondaryAccounts().at(1));
    assertEqual(result, penguin.get(), persistent.secondaryAccounts().at(2));
  });
}

void saveSavesAccounts(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    debit(bank, "giraffe", {});
    debit(bank, "penguin", {});
    debit(bank, "leopard", {});
    PersistentMemoryStub persistentMemory;
    bank.save(persistentMemory);
    assertEqual(result, masterAccount.get(), persistentMemory.primaryAccount());
    assertContainsSecondaryAccount(result, persistentMemory, giraffe);
    assertContainsSecondaryAccount(result, persistentMemory, penguin);
    assertContainsSecondaryAccount(result, persistentMemory, leopard);
  });
}

void loadLoadsAccounts(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    PersistentMemoryStub persistentMemory;
    bank.load(persistentMemory);
    assertEqual(result, &bank, persistentMemory.observer());
    AccountDeserializationStub deserialization;
    bank.notifyThatPrimaryAccountIsReady(deserialization, "giraffe");
    assertEqual(result, &deserialization, giraffe->deserialization());
    bank.notifyThatSecondaryAccountIsReady(deserialization, "penguin");
    assertEqual(result, &deserialization, penguin->deserialization());
    bank.notifyThatSecondaryAccountIsReady(deserialization, "leopard");
    assertEqual(result, &deserialization, leopard->deserialization());

    PersistentMemoryStub persistent;
    bank.save(persistent);
    assertEqual(result, giraffe.get(), persistent.primaryAccount());
    assertEqual(result, leopard.get(), persistent.secondaryAccounts().at(0));
    assertEqual(result, penguin.get(), persistent.secondaryAccounts().at(1));
  });
}

void removesDebitFromAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    debit(bank, "giraffe", {});
    bank.removeDebit("giraffe", Transaction{123_cents, "raccoon",
                                            Date{2013, Month::April, 3}});
    assertDebitRemoved(
        result, account,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void doesNothingWhenRemovingDebitFromNonexistentAccount(
    testcpplite::TestResult &) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    bank.removeDebit("giraffe", Transaction{123_cents, "raccoon",
                                            Date{2013, Month::April, 3}});
  });
}

void removesFromMasterAccountWhenRemovingCredit(
    testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    bank.removeCredit(
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertCreditRemoved(
        result, masterAccount,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void removeTransferRemovesDebitFromMasterAndCreditFromOther(
    testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    debit(bank, "giraffe", {});
    bank.removeTransfer("giraffe", 456_cents, Date{1776, Month::July, 4});
    assertCreditRemoved(result, account,
                        Transaction{456_cents, "transfer from master",
                                    Date{1776, Month::July, 4}});
    assertDebitRemoved(result, masterAccount,
                       Transaction{456_cents, "transfer to giraffe",
                                   Date{1776, Month::July, 4}});
  });
}

void renameAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    debit(bank, "giraffe", {});
    bank.renameAccount("giraffe", "zebra");
    assertEqual(result, "zebra", giraffe->newName());
  });
}

void verifiesDebitForExistingAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    debit(bank, "giraffe", {});
    bank.verifyDebit("giraffe", {1_cents, "hi", Date{2020, Month::April, 1}});
    assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                giraffe->debitToVerify());
  });
}

void verifiesCreditForMasterAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    bank.verifyCredit({1_cents, "hi", Date{2020, Month::April, 1}});
    assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                masterAccount->creditToVerify());
  });
}

void transferVerifiesTransactionsByDefault(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    bank.transferTo("giraffe", 456_cents, Date{1776, Month::July, 4});
    assertEqual(result,
                {456_cents, "transfer from master", Date{1776, Month::July, 4}},
                account->creditToVerify());
    assertEqual(result,
                {456_cents, "transfer to giraffe", Date{1776, Month::July, 4}},
                masterAccount->debitToVerify());
  });
}

void notifiesObserverOfNewAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    BankObserverStub observer;
    bank.attach(&observer);
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    AccountDeserializationStub deserialization;
    bank.notifyThatSecondaryAccountIsReady(deserialization, "giraffe");
    assertEqual(result, "giraffe", observer.newAccountName());
    assertEqual(result, account.get(), observer.newAccount());
  });
}

static void assertReduced(testcpplite::TestResult &result, const Date &date,
                          const std::shared_ptr<AccountStub> &account) {
  assertEqual(result, date, account->reducedDate());
}

void reduceReducesEachAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    debit(bank, "giraffe", {});
    debit(bank, "penguin", {});
    debit(bank, "leopard", {});
    bank.reduce(Date{2021, Month::March, 13});
    assertReduced(result, Date{2021, Month::March, 13}, masterAccount);
    assertReduced(result, Date{2021, Month::March, 13}, giraffe);
    assertReduced(result, Date{2021, Month::March, 13}, penguin);
    assertReduced(result, Date{2021, Month::March, 13}, leopard);
  });
}

void notifiesThatTotalBalanceHasChangedOnCredit(
    testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    BankObserverStub observer;
    bank.attach(&observer);
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    debit(bank, "giraffe", {});
    debit(bank, "penguin", {});
    debit(bank, "leopard", {});
    masterAccount->setBalance(456_cents);
    giraffe->setBalance(123_cents);
    penguin->setBalance(789_cents);
    leopard->setBalance(1111_cents);
    credit(bank);
    assertEqual(result, 456_cents + 123_cents + 789_cents + 1111_cents,
                observer.totalBalance());
  });
}

void removesAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    debit(bank, "giraffe", {});
    debit(bank, "penguin", {});
    debit(bank, "leopard", {});
    bank.removeAccount("giraffe");
    PersistentMemoryStub persistent;
    bank.save(persistent);
    assertEqual(result, masterAccount.get(), persistent.primaryAccount());
    assertEqual(result, leopard.get(), persistent.secondaryAccounts().at(0));
    assertEqual(result, penguin.get(), persistent.secondaryAccounts().at(1));
  });
}

void notifiesObserverOfRemovedAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &bank) {
    BankObserverStub observer;
    bank.attach(&observer);
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    debit(bank, "giraffe", {});
    bank.removeAccount("giraffe");
    assertTrue(result, account->removed());
  });
}

void notifiesThatTotalBalanceHasChangedOnRemoveAccount(
    testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    BankObserverStub observer;
    bank.attach(&observer);
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    debit(bank, "giraffe", {});
    debit(bank, "penguin", {});
    debit(bank, "leopard", {});
    masterAccount->setBalance(456_cents);
    giraffe->setBalance(123_cents);
    penguin->setBalance(789_cents);
    leopard->setBalance(1111_cents);
    bank.removeAccount("penguin");
    assertEqual(result, 456_cents + 123_cents + 1111_cents,
                observer.totalBalance());
  });
}

void createsAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, BudgetInMemory &budget) {
    budget.createAccount("panda");
    assertEqual(result, "panda", factory.name());
  });
}

void closesAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    debit(bank, "giraffe", {});
    debit(bank, "penguin", {});
    debit(bank, "leopard", {});
    giraffe->setBalance(123_cents);
    bank.closeAccount("giraffe", Date{2021, Month::April, 3});
    PersistentMemoryStub persistent;
    bank.save(persistent);
    assertEqual(result, {leopard.get(), penguin.get()},
                persistent.secondaryAccounts());
    assertEqual(result,
                {123_cents, "close giraffe", Date{2021, Month::April, 3}},
                masterAccount->creditedTransaction());
    assertEqual(result,
                {123_cents, "close giraffe", Date{2021, Month::April, 3}},
                masterAccount->creditToVerify());
  });
}

void closesAccountHavingNegativeBalance(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount,
               BudgetInMemory &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    debit(bank, "giraffe", {});
    giraffe->setBalance(-123_cents);
    bank.closeAccount("giraffe", Date{2021, Month::April, 3});
    assertEqual(result,
                {123_cents, "close giraffe", Date{2021, Month::April, 3}},
                masterAccount->debitedTransaction());
    assertEqual(result,
                {123_cents, "close giraffe", Date{2021, Month::April, 3}},
                masterAccount->debitToVerify());
  });
}
} // namespace sbash64::budget::bank
