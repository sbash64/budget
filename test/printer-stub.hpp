#ifndef SBASH64_BUDGET_TEST_PRINTER_STUB_HPP_
#define SBASH64_BUDGET_TEST_PRINTER_STUB_HPP_

#include <sbash64/budget/print.hpp>

namespace sbash64::budget {
class ViewStub : public View {
public:
  auto accountBalance() -> USD { return accountBalance_; }

  auto accountTransactions() -> std::vector<PrintableTransaction> {
    return accountTransactions_;
  }

  void printAccountSummary(
      std::string_view name, USD balance,
      const std::vector<PrintableTransaction> &transactions) override {
    accountName_ = name;
    accountTransactions_ = transactions;
    accountBalance_ = balance;
  }

  auto accountName() -> std::string { return accountName_; }

  auto primaryAccount() -> const Account * { return primaryAccount_; }

  void print(Account &primary,
             const std::vector<Account *> &secondaries) override {
    primaryAccount_ = &primary;
    secondaryAccounts_ = secondaries;
  }

  auto secondaryAccounts() -> std::vector<Account *> {
    return secondaryAccounts_;
  }

private:
  std::vector<PrintableTransaction> accountTransactions_;
  USD accountBalance_{};
  std::string accountName_;
  std::vector<Account *> secondaryAccounts_;
  const Account *primaryAccount_{};
};
} // namespace sbash64::budget

#endif
