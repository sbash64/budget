#ifndef SBASH64_BUDGET_TEST_ACCOUNT_STUB_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_STUB_HPP_

#include <sbash64/budget/domain.hpp>

namespace sbash64::budget {
class AccountStub : public virtual Account {
public:
  auto decreasedAllocationAmount() -> USD { return decreasedAllocationAmount_; }

  auto increasedAllocationAmount() -> USD { return increasedAllocationAmount_; }

  auto decreasedAllocationAmounts() -> std::vector<USD> {
    return decreasedAllocationAmounts_;
  }

  void increaseAllocationBy(USD usd) override {
    increasedAllocationAmount_ = usd;
  }

  void decreaseAllocationBy(USD usd) override {
    decreasedAllocationAmount_ = usd;
    decreasedAllocationAmounts_.push_back(usd);
  }

  void notifyThatAllocatedIsReady(USD) override {}

  void clear() override { cleared_ = true; }

  void setBalance(USD b) { balance_ = b; }

  void setAllocated(USD usd) { allocated_ = usd; }

  auto allocated() -> USD override { return allocated_; }

  void attach(Observer *) override {}

  void save(AccountSerialization &) override {}

  void load(AccountDeserialization &a) override { deserialization_ = &a; }

  auto deserialization() -> const AccountDeserialization * {
    return deserialization_;
  }

  void increaseAllocationByResolvingVerifiedTransactions() override {
    increasedAllocationByResolvingVerifiedTransactions_ = true;
  }

  void decreaseAllocationByResolvingVerifiedTransactions() override {
    decreasedAllocationByResolvingVerifiedTransactions_ = true;
  }

  [[nodiscard]] auto increasedAllocationByResolvingVerifiedTransactions() const
      -> bool {
    return increasedAllocationByResolvingVerifiedTransactions_;
  }

  [[nodiscard]] auto decreasedAllocationByResolvingVerifiedTransactions() const
      -> bool {
    return decreasedAllocationByResolvingVerifiedTransactions_;
  }

  auto balance() -> USD override { return balance_; }

  void remove() override { removed_ = true; }

  [[nodiscard]] auto removed() const -> bool { return removed_; }

  [[nodiscard]] auto cleared() const -> bool { return cleared_; }

  void notifyThatIsReady(TransactionDeserialization &) override {}

  void add(const Transaction &t) override { addedTransaction_ = t; }

  void remove(const Transaction &t) override {
    transactionRemoved_ = true;
    removedTransaction_ = t;
  }

  [[nodiscard]] auto transactionRemoved() const -> bool {
    return transactionRemoved_;
  }

  auto verifiedTransaction() -> Transaction { return verifiedTransaction_; }

  void verify(const Transaction &t) override { verifiedTransaction_ = t; }

  auto addedTransaction() -> Transaction { return addedTransaction_; }

  auto removedTransaction() -> Transaction { return removedTransaction_; }

private:
  Transaction verifiedTransaction_;
  Transaction addedTransaction_;
  Transaction removedTransaction_;
  std::vector<USD> decreasedAllocationAmounts_;
  const AccountDeserialization *deserialization_{};
  USD balance_{};
  USD balanceOnTransactionArchive{};
  USD increasedAllocationAmount_{};
  USD decreasedAllocationAmount_{};
  USD allocated_{};
  bool transactionRemoved_{};
  bool removed_{};
  bool cleared_{};
  bool increasedAllocationByResolvingVerifiedTransactions_{};
  bool decreasedAllocationByResolvingVerifiedTransactions_{};
};
} // namespace sbash64::budget

#endif