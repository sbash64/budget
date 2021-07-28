#ifndef SBASH64_BUDGET_TRANSACTION_HPP_
#define SBASH64_BUDGET_TRANSACTION_HPP_

#include "domain.hpp"

namespace sbash64::budget {
class ObservableTransactionInMemory : public ObservableTransaction {
public:
  void attach(Observer *) override;
  void initialize(const Transaction &) override;
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
