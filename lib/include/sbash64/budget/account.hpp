#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "domain.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public Account {
public:
  explicit InMemoryAccount(std::string name, ObservableTransaction::Factory &);
  void attach(Observer *) override;
  void credit(const Transaction &) override;
  void debit(const Transaction &) override;
  void save(AccountSerialization &) override;
  void load(AccountDeserialization &) override;
  void removeCredit(const Transaction &) override;
  void removeDebit(const Transaction &) override;
  void rename(std::string_view) override;
  void verifyDebit(const Transaction &) override;
  void verifyCredit(const Transaction &) override;
  void notifyThatCreditIsReady(TransactionDeserialization &) override;
  void notifyThatDebitIsReady(TransactionDeserialization &) override;
  void reduce() override;
  auto balance() -> USD override;
  void remove() override;
  void clear() override;
  void withdraw(USD) override;

  class Factory : public Account::Factory {
  public:
    explicit Factory(
        ObservableTransaction::Factory &observableTransactionFactory);
    auto make(std::string_view name) -> std::shared_ptr<Account> override;

  private:
    ObservableTransaction::Factory &observableTransactionFactory;
  };

private:
  std::vector<std::shared_ptr<ObservableTransaction>> creditRecords;
  std::vector<std::shared_ptr<ObservableTransaction>> debitRecords;
  std::string name;
  Observer *observer{};
  USD funds_{};
  ObservableTransaction::Factory &factory;
};
} // namespace sbash64::budget

#endif
