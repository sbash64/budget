#ifndef SBASH64_BUDGET_BANK_HPP_
#define SBASH64_BUDGET_BANK_HPP_

#include "budget.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sbash64::budget {
class Bank : public Model {
public:
  explicit Bank(Account::Factory &);
  void debit(std::string_view accountName, const Transaction &) override;
  void removeDebit(std::string_view accountName, const Transaction &) override;
  void credit(const Transaction &) override;
  void removeCredit(const Transaction &) override;
  void transferTo(std::string_view accountName, USD amount, Date) override;
  void removeTransfer(std::string_view accountName, USD amount, Date) override;
  void show(View &) override;
  void save(SessionSerialization &) override;
  void load(SessionDeserialization &) override;
  void renameAccount(std::string_view from, std::string_view to) override;

private:
  Account::Factory &factory;
  std::shared_ptr<Account> masterAccount;
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
};

using Transactions = std::vector<Transaction>;

class InMemoryAccount : public Account {
public:
  explicit InMemoryAccount(std::string name);
  void credit(const Transaction &) override;
  void debit(const Transaction &) override;
  void show(View &) override;
  void save(SessionSerialization &) override;
  void load(SessionDeserialization &) override;
  void removeCredit(const Transaction &) override;
  void removeDebit(const Transaction &) override;
  void rename(std::string_view) override;
  void verifyDebit(const Transaction &);
  void verifyCredit(const Transaction &);
  auto findUnverifiedDebits(USD amount) -> Transactions;
  auto findUnverifiedCredits(USD amount) -> Transactions;

  class Factory : public Account::Factory {
  public:
    auto make(std::string_view name) -> std::shared_ptr<Account> override;
  };

private:
  VerifiableTransactions debits;
  VerifiableTransactions credits;
  std::string name;
};
} // namespace sbash64::budget

#endif
