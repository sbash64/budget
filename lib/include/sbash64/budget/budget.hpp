#ifndef SBASH64_BUDGET_BUDGET_HPP_
#define SBASH64_BUDGET_BUDGET_HPP_

#include "domain.hpp"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace sbash64::budget {
constexpr const std::array<char, 7> masterAccountName{"master"};

class BudgetInMemory : public Budget {
public:
  explicit BudgetInMemory(Account &, Account::Factory &);
  void attach(Observer *) override;
  void debit(std::string_view accountName, const Transaction &) override;
  void removeDebit(std::string_view accountName, const Transaction &) override;
  void credit(const Transaction &) override;
  void removeCredit(const Transaction &) override;
  void transferTo(std::string_view accountName, USD amount) override;
  void removeAccount(std::string_view) override;
  void save(BudgetSerialization &) override;
  void load(BudgetDeserialization &) override;
  void renameAccount(std::string_view from, std::string_view to) override;
  void verifyDebit(std::string_view accountName, const Transaction &) override;
  void verifyCredit(const Transaction &) override;
  void notifyThatPrimaryAccountIsReady(AccountDeserialization &,
                                       std::string_view name) override;
  void notifyThatSecondaryAccountIsReady(AccountDeserialization &,
                                         std::string_view name) override;
  void reduce(const Date &) override;
  void createAccount(std::string_view name) override;
  void closeAccount(std::string_view name) override;
  void allocate(std::string_view accountName, USD, const Date &) override;
  void restore(const Date &) override;

private:
  Account::Factory &accountFactory;
  Observer *observer{};
  Account &incomeAccount;
  std::map<std::string, std::shared_ptr<Account>, std::less<>> expenseAccounts;
};
} // namespace sbash64::budget

#endif
