#ifndef SBASH64_BUDGET_BANK_HPP_
#define SBASH64_BUDGET_BANK_HPP_

#include "budget.hpp"
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace sbash64::budget {
constexpr const std::array<char, 7> masterAccountName{"master"};

class Bank : public Model {
public:
  Bank(Account::Factory &, TransactionRecord::Factory &);
  void attach(Observer *) override;
  void debit(std::string_view accountName, const Transaction &) override;
  void removeDebit(std::string_view accountName, const Transaction &) override;
  void credit(const Transaction &) override;
  void removeCredit(const Transaction &) override;
  void transferTo(std::string_view accountName, USD amount, Date) override;
  void removeTransfer(std::string_view accountName, USD amount, Date) override;
  void removeAccount(std::string_view) override;
  void show(View &) override;
  void save(SessionSerialization &) override;
  void load(SessionDeserialization &) override;
  void renameAccount(std::string_view from, std::string_view to) override;
  auto findUnverifiedDebits(std::string_view accountName, USD amount)
      -> Transactions override;
  auto findUnverifiedCredits(USD amount) -> Transactions override;
  void verifyDebit(std::string_view accountName, const Transaction &) override;
  void verifyCredit(const Transaction &) override;
  void notifyThatPrimaryAccountIsReady(AccountDeserialization &,
                                       std::string_view name) override;
  void notifyThatSecondaryAccountIsReady(AccountDeserialization &,
                                         std::string_view name) override;
  void reduce(const Date &) override;

private:
  Account::Factory &factory;
  TransactionRecord::Factory &transactionRecordFactory;
  Observer *observer{};
  std::shared_ptr<Account> primaryAccount;
  std::map<std::string, std::shared_ptr<Account>, std::less<>>
      secondaryAccounts;
};
} // namespace sbash64::budget

#endif
