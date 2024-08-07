#ifndef SBASH64_BUDGET_TRANSACTION_HPP_
#define SBASH64_BUDGET_TRANSACTION_HPP_

#include "domain.hpp"

#include <functional>
#include <vector>

namespace sbash64::budget {
class ObservableTransactionInMemory : public ObservableTransaction {
public:
  void attach(Observer &) override;
  void initialize(const Transaction &) override;
  auto verifies(const Transaction &) -> bool override;
  auto removes(const Transaction &) -> bool override;
  void save(TransactionSerialization &) override;
  auto amount() -> USD override;
  void remove() override;
  void archive() override;
  auto verified() -> bool override;

  class Factory : public ObservableTransaction::Factory {
  public:
    auto make() -> std::shared_ptr<ObservableTransaction> override;
  };

private:
  ArchivableVerifiableTransaction archivableVerifiableTransaction;
  std::vector<std::reference_wrapper<Observer>> observers{};
};
} // namespace sbash64::budget

#endif
