#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "domain.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public virtual Account {
public:
  InMemoryAccount(std::string name, ObservableTransaction::Factory &);
  void attach(Observer *) override;
  void rename(std::string_view) override;
  void withdraw(USD) override;
  void deposit(USD) override;
  void reduce() override;
  void remove() override;
  void clear() override;
  void save(AccountSerialization &) override;
  void load(AccountDeserialization &) override;
  void notifyThatCreditIsReady(TransactionDeserialization &) override;
  void notifyThatDebitIsReady(TransactionDeserialization &) override;
  void notifyThatFundsAreReady(USD) override;

protected:
  std::vector<std::shared_ptr<ObservableTransaction>> creditRecords;
  std::vector<std::shared_ptr<ObservableTransaction>> debitRecords;
  std::string name;
  Observer *observer{};
  USD funds{};
  ObservableTransaction::Factory &factory;
};

class InMemoryExpenseAccount : public InMemoryAccount, public ExpenseAccount {
public:
  using Account::remove;
  using InMemoryAccount::InMemoryAccount;
  void add(const Transaction &) override;
  void verify(const Transaction &) override;
  void remove(const Transaction &) override;
  auto balance() -> USD override;

  class Factory : public ExpenseAccount::Factory {
  public:
    explicit Factory(ObservableTransaction::Factory &);
    auto make(std::string_view name)
        -> std::shared_ptr<ExpenseAccount> override;

  private:
    ObservableTransaction::Factory &transactionFactory;
  };
};

class InMemoryIncomeAccount : public InMemoryAccount, public IncomeAccount {
public:
  using Account::remove;
  using InMemoryAccount::InMemoryAccount;
  void add(const Transaction &) override;
  void verify(const Transaction &) override;
  void remove(const Transaction &) override;
  auto balance() -> USD override;
};
} // namespace sbash64::budget

#endif
