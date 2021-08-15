#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "domain.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public virtual Account, public IncomeAccount {
public:
  InMemoryAccount(std::string name, ObservableTransaction::Factory &);
  void attach(Observer *) override;
  void credit(const Transaction &) override;
  void debit(const Transaction &) override;
  void removeCredit(const Transaction &) override;
  void removeDebit(const Transaction &) override;
  void verifyCredit(const Transaction &) override;
  void verifyDebit(const Transaction &) override;
  void rename(std::string_view) override;
  auto balance() -> USD override;
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

  class Factory : public Account::Factory {
  public:
    explicit Factory(ObservableTransaction::Factory &);
    auto make(std::string_view name) -> std::shared_ptr<Account> override;

  private:
    ObservableTransaction::Factory &transactionFactory;
  };

private:
  std::vector<std::shared_ptr<ObservableTransaction>> creditRecords;
  std::vector<std::shared_ptr<ObservableTransaction>> debitRecords;
  std::string name;
  Observer *observer{};
  USD funds{};
  ObservableTransaction::Factory &factory;
};
} // namespace sbash64::budget

#endif
