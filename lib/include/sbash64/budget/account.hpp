#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "domain.hpp"

#include <memory>
#include <vector>

namespace sbash64::budget {
class AccountInMemory : public Account {
public:
  using TransactionsType = std::vector<std::shared_ptr<ObservableTransaction>>;

  explicit AccountInMemory(ObservableTransaction::Factory &);
  void attach(Observer *) override;
  void remove() override;
  void load(AccountDeserialization &) override;
  void clear() override;
  void increaseAllocationBy(USD) override;
  void decreaseAllocationBy(USD) override;
  auto allocated() -> USD override;
  void notifyThatAllocatedIsReady(USD) override;
  void increaseAllocationByResolvingVerifiedTransactions() override;
  void decreaseAllocationByResolvingVerifiedTransactions() override;
  void notifyThatIsReady(TransactionDeserialization &) override;
  void save(AccountSerialization &) override;
  void add(const Transaction &) override;
  void verify(const Transaction &) override;
  void remove(const Transaction &) override;
  auto balance() -> USD override;

  class Factory : public Account::Factory {
  public:
    explicit Factory(ObservableTransaction::Factory &);
    auto make() -> std::shared_ptr<Account> override;

  private:
    ObservableTransaction::Factory &transactionFactory;
  };

private:
  TransactionsType transactions;
  Observer *observer{};
  ObservableTransaction::Factory &factory;
  USD allocation{};
};
} // namespace sbash64::budget

#endif
