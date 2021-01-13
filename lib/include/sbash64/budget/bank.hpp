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
  void removeDebit(std::string_view accountName, const Transaction &);
  void credit(const Transaction &) override;
  void removeCredit(const Transaction &);
  void transferTo(std::string_view accountName, USD amount, Date) override;
  void removeTransfer(std::string_view accountName, USD amount, Date);
  void show(View &) override;
  void save(SessionSerialization &) override;
  void load(SessionDeserialization &) override;

private:
  Account::Factory &factory;
  std::shared_ptr<Account> masterAccount;
  std::map<std::string, std::shared_ptr<Account>, std::less<>> accounts;
};

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

  class Factory : public Account::Factory {
  public:
    auto make(std::string_view name) -> std::shared_ptr<Account> override;
  };

private:
  Transactions debits;
  Transactions credits;
  std::string name;
};
} // namespace sbash64::budget

#endif
