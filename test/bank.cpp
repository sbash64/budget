#include "bank.hpp"
#include "persistent-memory-stub.hpp"
#include "usd.hpp"
#include "view-stub.hpp"
#include <functional>
#include <map>
#include <sbash64/budget/bank.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <utility>

namespace sbash64::budget::bank {
namespace {
class AccountStub : public Account {
public:
  auto creditedTransaction() -> Transaction { return creditedTransaction_; }

  void credit(const Transaction &t) override { creditedTransaction_ = t; }

  auto debitedTransaction() -> Transaction { return debitedTransaction_; }

  void debit(const Transaction &t) override { debitedTransaction_ = t; }

  auto removedDebit() -> Transaction { return removedDebit_; }

  void removeDebit(const Transaction &t) override { removedDebit_ = t; }

  auto removedCredit() -> Transaction { return removedCredit_; }

  void removeCredit(const Transaction &t) override { removedCredit_ = t; }

  void show(View &) override {}

  void save(SessionSerialization &) override {}

  void load(SessionDeserialization &) override {}

  auto newName() -> std::string { return newName_; }

  void rename(std::string_view s) override { newName_ = s; }

  void setFoundUnverifiedDebits(Transactions t) {
    foundUnverifiedDebits = std::move(t);
  }

  void setFoundUnverifiedCredits(Transactions t) {
    foundUnverifiedCredits = std::move(t);
  }

  auto findUnverifiedDebits(USD amount) -> Transactions override {
    findUnverifiedDebitsAmount_ = amount;
    return foundUnverifiedDebits;
  }

  auto findUnverifiedCredits(USD amount) -> Transactions {
    findUnverifiedCreditsAmount_ = amount;
    return foundUnverifiedCredits;
  }

  auto findUnverifiedDebitsAmount() -> USD {
    return findUnverifiedDebitsAmount_;
  }

  auto findUnverifiedCreditsAmount() -> USD {
    return findUnverifiedCreditsAmount_;
  }

  auto debitToVerify() -> Transaction { return debitToVerify_; }

  void verifyDebit(const Transaction &t) override { debitToVerify_ = t; }

  auto creditToVerify() -> Transaction { return creditToVerify_; }

  void verifyCredit(const Transaction &t) override { creditToVerify_ = t; }

private:
  Transaction creditToVerify_;
  Transaction debitToVerify_;
  Transaction creditedTransaction_;
  Transaction debitedTransaction_;
  Transaction removedDebit_;
  Transaction removedCredit_;
  Transactions foundUnverifiedDebits;
  Transactions foundUnverifiedCredits;
  std::string newName_;
  USD findUnverifiedDebitsAmount_{};
  USD findUnverifiedCreditsAmount_{};
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
} // namespace

static void add(AccountFactoryStub &factory, std::shared_ptr<Account> account,
                std::string_view accountName) {
  factory.add(std::move(account), accountName);
}

static void testBank(
    const std::function<void(AccountFactoryStub &factory,
                             const std::shared_ptr<AccountStub> &masterAccount,
                             Bank &bank)> &f) {
  AccountFactoryStub factory;
  const auto masterAccount{std::make_shared<AccountStub>()};
  add(factory, masterAccount, "master");
  Bank bank{factory};
  f(factory, masterAccount, bank);
}

static void debit(Bank &bank, std::string_view accountName,
                  const Transaction &t) {
  bank.debit(accountName, t);
}

static void assertContains(testcpplite::TestResult &result,
                           const std::vector<Account *> &accounts,
                           const std::shared_ptr<Account> &account) {
  assertTrue(result, std::find(accounts.begin(), accounts.end(),
                               account.get()) != accounts.end());
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
               const std::shared_ptr<AccountStub> &,
               Bank &) { assertEqual(result, "master", factory.name()); });
}

void creditsMasterAccountWhenCredited(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &,
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
    bank.credit(Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertCredited(
        result, masterAccount,
        Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  });
}

void createsAccountWhenDebitingNonexistent(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, Bank &bank) {
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
               const std::shared_ptr<AccountStub> &, Bank &bank) {
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
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
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
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    const auto penguin{std::make_shared<AccountStub>()};
    add(factory, penguin, "penguin");
    const auto leopard{std::make_shared<AccountStub>()};
    add(factory, leopard, "leopard");
    debit(bank, "giraffe", {});
    debit(bank, "penguin", {});
    debit(bank, "leopard", {});
    ViewStub view;
    bank.show(view);
    assertEqual(result, masterAccount.get(), view.primaryAccount());
    assertEqual(result, giraffe.get(), view.secondaryAccounts().at(0));
    assertEqual(result, leopard.get(), view.secondaryAccounts().at(1));
    assertEqual(result, penguin.get(), view.secondaryAccounts().at(2));
  });
}

void saveSavesAccounts(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
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
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
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
    bank.load(persistentMemory);
    assertEqual(result, &factory, persistentMemory.accountFactory());
    assertEqual(result, masterAccount.get(),
                persistentMemory.primaryAccountToLoadInto()->get());
    assertEqual(
        result, giraffe.get(),
        persistentMemory.secondaryAccountsToLoadInto()->at("giraffe").get());
    assertEqual(
        result, penguin.get(),
        persistentMemory.secondaryAccountsToLoadInto()->at("penguin").get());
    assertEqual(
        result, leopard.get(),
        persistentMemory.secondaryAccountsToLoadInto()->at("leopard").get());
  });
}

void removesDebitFromAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, Bank &bank) {
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
               const std::shared_ptr<AccountStub> &, Bank &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    bank.removeDebit("giraffe", Transaction{123_cents, "raccoon",
                                            Date{2013, Month::April, 3}});
  });
}

void removesFromMasterAccountWhenRemovingCredit(
    testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &,
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
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
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
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
               const std::shared_ptr<AccountStub> &, Bank &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    debit(bank, "giraffe", {});
    bank.renameAccount("giraffe", "zebra");
    assertEqual(result, "zebra", giraffe->newName());
  });
}

void findsUnverifiedDebitsFromAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, Bank &bank) {
    const auto giraffe{std::make_shared<AccountStub>()};
    add(factory, giraffe, "giraffe");
    debit(bank, "giraffe", {});
    giraffe->setFoundUnverifiedDebits(
        {{1_cents, "hi", Date{2020, Month::April, 1}},
         {2_cents, "nye", Date{2020, Month::August, 2}},
         {3_cents, "sigh", Date{2020, Month::December, 3}}});
    assertEqual(result,
                {{1_cents, "hi", Date{2020, Month::April, 1}},
                 {2_cents, "nye", Date{2020, Month::August, 2}},
                 {3_cents, "sigh", Date{2020, Month::December, 3}}},
                bank.findUnverifiedDebits("giraffe", 123_cents));
    assertEqual(result, 123_cents, giraffe->findUnverifiedDebitsAmount());
  });
}

void findsUnverifiedCreditsFromMasterAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &,
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
    masterAccount->setFoundUnverifiedCredits(
        {{1_cents, "hi", Date{2020, Month::April, 1}},
         {2_cents, "nye", Date{2020, Month::August, 2}},
         {3_cents, "sigh", Date{2020, Month::December, 3}}});
    assertEqual(result,
                {{1_cents, "hi", Date{2020, Month::April, 1}},
                 {2_cents, "nye", Date{2020, Month::August, 2}},
                 {3_cents, "sigh", Date{2020, Month::December, 3}}},
                bank.findUnverifiedCredits(123_cents));
    assertEqual(result, 123_cents,
                masterAccount->findUnverifiedCreditsAmount());
  });
}

void verifiesDebitForExistingAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory,
               const std::shared_ptr<AccountStub> &, Bank &bank) {
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
               const std::shared_ptr<AccountStub> &masterAccount, Bank &bank) {
    bank.verifyCredit({1_cents, "hi", Date{2020, Month::April, 1}});
    assertEqual(result, {1_cents, "hi", Date{2020, Month::April, 1}},
                masterAccount->creditToVerify());
  });
}
} // namespace sbash64::budget::bank
