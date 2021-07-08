#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "budget.hpp"
#include <memory>
#include <string>
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public Account {
public:
  explicit InMemoryAccount(std::string name, TransactionRecord::Factory &);
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
  void notifyThatCreditIsReady(TransactionRecordDeserialization &) override;
  void notifyThatDebitIsReady(TransactionRecordDeserialization &) override;
  void reduce(const Date &) override;
  auto balance() -> USD override;
  void remove() override {}

  class Factory : public Account::Factory {
  public:
    auto make(std::string_view name, TransactionRecord::Factory &)
        -> std::shared_ptr<Account> override;
  };

private:
  std::vector<std::shared_ptr<TransactionRecord>> creditRecords;
  std::vector<std::shared_ptr<TransactionRecord>> debitRecords;
  std::string name;
  Observer *observer{};
  TransactionRecord::Factory &factory;
};

class TransactionRecordInMemory : public TransactionRecord {
public:
  void attach(Observer *a) override { observer = a; }

  void initialize(const Transaction &t) override {
    verifiableTransaction.transaction = t;
  }

  void verify() override {}

  auto verifies(const Transaction &t) -> bool override {
    if (!verifiableTransaction.verified &&
        verifiableTransaction.transaction == t) {
      if (observer != nullptr)
        observer->notifyThatIsVerified();
      return verifiableTransaction.verified = true;
    }
    return false;
  }

  auto removes(const Transaction &t) -> bool override {
    if (verifiableTransaction.transaction == t) {
      if (observer != nullptr)
        observer->notifyThatWillBeRemoved();
      return true;
    }
    return false;
  }

  void save(TransactionRecordSerialization &) override {}
  void load(TransactionRecordDeserialization &) override {}
  auto amount() -> USD override {
    return verifiableTransaction.transaction.amount;
  }
  void ready(const VerifiableTransaction &) override {}
  void remove() override {}

private:
  VerifiableTransaction verifiableTransaction;
  Observer *observer{};
};
} // namespace sbash64::budget

#endif
