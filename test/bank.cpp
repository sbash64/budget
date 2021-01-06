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

  void print(View &) override {}

private:
  Transaction creditedTransaction_;
  Transaction debitedTransaction_;
};

class AccountFactoryStub : public AccountFactory {
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

static void testBank(
    const std::function<void(AccountFactoryStub &factory, Bank &bank)> &f) {
  AccountFactoryStub factory;
  Bank bank{factory};
  f(factory, bank);
}

static void debit(Bank &bank, std::string_view accountName,
                  const Transaction &t) {
  bank.debit(accountName, t);
}

static void add(AccountFactoryStub &factory, std::shared_ptr<Account> account,
                std::string_view accountName) {
  factory.add(std::move(account), accountName);
}

static void
assertContainsSecondaryAccount(testcpplite::TestResult &result,
                               PersistentMemoryStub &persistentMemory,
                               const std::shared_ptr<Account> &account) {
  assertTrue(result, std::find(persistentMemory.secondaryAccounts().begin(),
                               persistentMemory.secondaryAccounts().end(),
                               account.get()) !=
                         persistentMemory.secondaryAccounts().end());
}

void createsMasterAccountOnConstruction(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory, Bank &) {
    assertEqual(result, "master", factory.name());
  });
}

void creditsMasterAccountWhenCredited(testcpplite::TestResult &result) {
  AccountFactoryStub factory;
  const auto masterAccount{std::make_shared<AccountStub>()};
  add(factory, masterAccount, "master");
  Bank bank{factory};
  bank.credit(Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  assertEqual(result,
              Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}},
              masterAccount->creditedTransaction());
}

void debitsNonexistantAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory, Bank &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    debit(bank, "giraffe",
          Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    assertEqual(result,
                Transaction{456_cents, "mouse", Date{2024, Month::August, 23}},
                account->debitedTransaction());
  });
}

void debitsExistingAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory, Bank &bank) {
    const auto account{std::make_shared<AccountStub>()};
    add(factory, account, "giraffe");
    debit(bank, "giraffe",
          Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    add(factory, nullptr, "giraffe");
    debit(bank, "giraffe",
          Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertEqual(result,
                Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}},
                account->debitedTransaction());
  });
}

void transferDebitsMasterAndCreditsOther(testcpplite::TestResult &result) {
  AccountFactoryStub factory;
  const auto masterAccount{std::make_shared<AccountStub>()};
  add(factory, masterAccount, "master");
  Bank bank{factory};
  const auto account{std::make_shared<AccountStub>()};
  add(factory, account, "giraffe");
  bank.transferTo("giraffe", 456_cents, Date{1776, Month::July, 4});
  assertEqual(result,
              Transaction{456_cents, "transfer from master",
                          Date{1776, Month::July, 4}},
              account->creditedTransaction());
  assertEqual(
      result,
      Transaction{456_cents, "transfer to giraffe", Date{1776, Month::July, 4}},
      masterAccount->debitedTransaction());
}

void printPrintsAccountsInAlphabeticOrder(testcpplite::TestResult &result) {
  AccountFactoryStub factory;
  const auto masterAccount{std::make_shared<AccountStub>()};
  add(factory, masterAccount, "master");
  Bank bank{factory};
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
  bank.print(view);
  assertEqual(result, masterAccount.get(), view.primaryAccount());
  assertEqual(result, giraffe.get(), view.secondaryAccounts().at(0));
  assertEqual(result, leopard.get(), view.secondaryAccounts().at(1));
  assertEqual(result, penguin.get(), view.secondaryAccounts().at(2));
}

void saveSavesAccounts(testcpplite::TestResult &result) {
  AccountFactoryStub factory;
  const auto masterAccount{std::make_shared<AccountStub>()};
  add(factory, masterAccount, "master");
  Bank bank{factory};
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
}
} // namespace sbash64::budget::bank
