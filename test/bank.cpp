#include "bank.hpp"
#include "usd.hpp"
#include <map>
#include <sbash64/budget/bank.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <utility>

namespace sbash64::budget::bank {
namespace {
class AccountStub : public Account {
public:
  auto creditedTransaction() -> Transaction { return creditedTransaction_; }

  void credit(const Transaction &t) { creditedTransaction_ = t; }

private:
  Transaction creditedTransaction_;
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

void createsMasterAccountOnConstruction(testcpplite::TestResult &result) {
  AccountFactoryStub factory;
  Bank bank{factory};
  assertEqual(result, "master", factory.name());
}

void creditsMasterAccountWhenCredited(testcpplite::TestResult &result) {
  AccountFactoryStub factory;
  auto masterAccount{std::make_shared<AccountStub>()};
  factory.add(masterAccount, "master");
  Bank bank{factory};
  bank.credit(Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}});
  assertEqual(result,
              Transaction{123_cents, "raccoon", Date{2013, Month::April, 3}},
              masterAccount->creditedTransaction());
}
} // namespace sbash64::budget::bank
