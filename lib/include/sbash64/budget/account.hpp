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
  void reduce(const Date &) override;
  auto balance() -> USD override;
  void remove() override;

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
  ObservableTransaction::Factory &factory;
};

class ObservableTransactionInMemory : public ObservableTransaction {
public:
  void attach(Observer *) override;
  void initialize(const Transaction &) override;
  void verify() override;
  auto verifies(const Transaction &) -> bool override;
  auto removes(const Transaction &) -> bool override;
  void save(TransactionSerialization &) override;
  void load(TransactionDeserialization &) override;
  auto amount() -> USD override;
  void ready(const VerifiableTransaction &) override;
  void remove() override;

  class Factory : public ObservableTransaction::Factory {
  public:
    auto make() -> std::shared_ptr<ObservableTransaction> override;
  };

private:
  VerifiableTransaction verifiableTransaction;
  Observer *observer{};
};
} // namespace sbash64::budget

#endif
