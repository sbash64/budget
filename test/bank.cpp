#include "bank.hpp"
#include "usd.hpp"
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

private:
  Transaction creditedTransaction_;
  Transaction debitedTransaction_;
};

class AccountFactoryStub : public AccountFactory {
public:
  void add(std::shared_ptr<Account> account, std::string_view name) {
    accounts[name.data()] = std::move(account);
  }

  auto name() -> std::string { return name_; }

  auto make(std::string_view s) -> std::shared_ptr<Account> override {
    name_ = s;
    return accounts.count(s.data()) == 0 ? nullptr : accounts.at(s.data());
  }

private:
  std::map<std::string, std::shared_ptr<Account>> accounts;
  std::string name_;
};
} // namespace

static void testBank(
    const std::function<void(AccountFactoryStub &factory, Bank &bank)> &f) {
  AccountFactoryStub factory;
  Bank bank{factory};
  f(factory, bank);
}

void createsMasterAccountOnConstruction(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory, Bank &) {
    assertEqual(result, "master", factory.name());
  });
}

void creditsMasterAccountWhenCredited(testcpplite::TestResult &result) {
  AccountFactoryStub factory;
  const auto masterAccount{std::make_shared<AccountStub>()};
  factory.add(masterAccount, "master");
  Bank bank{factory};
  bank.credit(Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  assertEqual(result,
              Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}},
              masterAccount->creditedTransaction());
}

void debitsNonexistantAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory, Bank &bank) {
    const auto account{std::make_shared<AccountStub>()};
    factory.add(account, "giraffe");
    bank.debit("giraffe",
               Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    assertEqual(result,
                Transaction{456_cents, "mouse", Date{2024, Month::August, 23}},
                account->debitedTransaction());
  });
}

void debitsExistingAccount(testcpplite::TestResult &result) {
  testBank([&](AccountFactoryStub &factory, Bank &bank) {
    const auto account{std::make_shared<AccountStub>()};
    factory.add(account, "giraffe");
    bank.debit("giraffe",
               Transaction{456_cents, "mouse", Date{2024, Month::August, 23}});
    factory.add(nullptr, "giraffe");
    bank.debit("giraffe",
               Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
    assertEqual(result,
                Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}},
                account->debitedTransaction());
  });
}
} // namespace sbash64::budget::bank
