#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "budget.hpp"
#include <map>
#include <memory>
#include <string>

namespace sbash64::budget {
class InMemoryAccount : public Account {
public:
  explicit InMemoryAccount(std::string name, TransactionRecord::Factory &);
  void attach(Observer *) override;
  void credit(const Transaction &) override;
  void debit(const Transaction &) override;
  void show(View &) override;
  void save(AccountSerialization &) override;
  void load(AccountDeserialization &) override;
  void removeCredit(const Transaction &) override;
  void removeDebit(const Transaction &) override;
  void rename(std::string_view) override;
  void verifyDebit(const Transaction &) override;
  void verifyCredit(const Transaction &) override;
  auto findUnverifiedDebits(USD amount) -> Transactions override;
  auto findUnverifiedCredits(USD amount) -> Transactions override;
  void
  notifyThatCreditHasBeenDeserialized(const VerifiableTransaction &) override;
  void
  notifyThatDebitHasBeenDeserialized(const VerifiableTransaction &) override;
  void reduce(const Date &) override;
  auto balance() -> USD override;

  class Factory : public Account::Factory {
  public:
    auto make(std::string_view name, TransactionRecord::Factory &)
        -> std::shared_ptr<Account> override;
  };

private:
  VerifiableTransactions debits;
  VerifiableTransactions credits;
  std::map<Transaction, std::shared_ptr<TransactionRecord>> creditRecords;
  std::map<Transaction, std::shared_ptr<TransactionRecord>> debitRecords;
  std::string name;
  Observer *observer{};
  TransactionRecord::Factory &factory;
};
} // namespace sbash64::budget

#endif
