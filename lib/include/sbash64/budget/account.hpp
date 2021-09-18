#ifndef SBASH64_BUDGET_ACCOUNT_HPP_
#define SBASH64_BUDGET_ACCOUNT_HPP_

#include "domain.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sbash64::budget {
class InMemoryAccount : public virtual Account {
public:
  explicit InMemoryAccount(ObservableTransaction::Factory &);
  void attach(Observer *) override;
  void remove() override;
  void load(AccountDeserialization &) override;
  void clear() override;
  void archiveVerifiedTransactions() override;
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
  std::vector<std::shared_ptr<ObservableTransaction>> transactions;
  Observer *observer{};
  ObservableTransaction::Factory &factory;
};
} // namespace sbash64::budget

#endif
