#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "domain.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public virtual Account {
public:
  InMemoryAccount(std::string name, ObservableTransaction::Factory &,
                  bool positive);
  void attach(Observer *) override;
  void rename(std::string_view) override;
  void withdraw(USD) override;
  void deposit(USD) override;
  void remove() override;
  void load(AccountDeserialization &) override;
  void notifyThatFundsAreReady(USD) override;
  void clear() override;
  void reduce() override;
  void notifyThatIsReady(TransactionDeserialization &) override;
  void save(AccountSerialization &) override;
  void add(const Transaction &) override;
  void verify(const Transaction &) override;
  void remove(const Transaction &) override;
  auto balance() -> USD override;

  class Factory : public Account::Factory {
  public:
    explicit Factory(ObservableTransaction::Factory &);
    auto make(std::string_view name) -> std::shared_ptr<Account> override;

  private:
    ObservableTransaction::Factory &transactionFactory;
  };

private:
  std::vector<std::shared_ptr<ObservableTransaction>> transactions;
  std::string name;
  Observer *observer{};
  USD funds{};
  ObservableTransaction::Factory &factory;
  bool positive;
};

class InMemoryExpenseAccount : public InMemoryAccount, public ExpenseAccount {
public:
  using InMemoryAccount::InMemoryAccount;

private:
};

class InMemoryIncomeAccount : public InMemoryAccount, public IncomeAccount {
public:
  using InMemoryAccount::InMemoryAccount;
};
} // namespace sbash64::budget

#endif
