#include "account.hpp"
#include <algorithm>
#include <memory>
#include <numeric>

namespace sbash64::budget {
void InMemoryAccount::credit(const Transaction &t) { credits.push_back(t); }

void InMemoryAccount::debit(const Transaction &t) { debits.push_back(t); }

static auto balance(const std::vector<Transaction> &transactions) -> USD {
  return accumulate(
      transactions.begin(), transactions.end(), USD{0},
      [](USD total, const Transaction &t) { return total + t.amount; });
}

static auto balance(const std::vector<Transaction> &credits,
                    const std::vector<Transaction> &debits) -> USD {
  return balance(credits) - balance(debits);
}

static auto
dateSortedPrintableTransactions(const std::vector<Transaction> &credits,
                                const std::vector<Transaction> &debits)
    -> std::vector<PrintableTransaction> {
  std::vector<PrintableTransaction> transactions;
  transactions.reserve(credits.size() + debits.size());
  for (const auto &c : credits)
    transactions.push_back(printableCredit(c));
  for (const auto &d : debits)
    transactions.push_back(printableDebit(d));
  sort(transactions.begin(), transactions.end(),
       [](const PrintableTransaction &a, const PrintableTransaction &b) {
         return a.transaction.date < b.transaction.date;
       });
  return transactions;
}

void InMemoryAccount::show(View &printer) {
  printer.showAccountSummary(name, balance(credits, debits),
                             dateSortedPrintableTransactions(credits, debits));
}

void InMemoryAccount::save(SessionSerialization &persistentMemory) {
  persistentMemory.saveAccount(name, credits, debits);
}

InMemoryAccount::InMemoryAccount(std::string name) : name{std::move(name)} {}

auto InMemoryAccount::Factory::make(std::string_view name)
    -> std::shared_ptr<Account> {
  return std::make_shared<InMemoryAccount>(std::string{name});
}

void InMemoryAccount::load(SessionDeserialization &persistentMemory) {
  persistentMemory.loadAccount(credits, debits);
}

static void remove(std::vector<Transaction> &transactions,
                   const Transaction &t) {
  const auto it{find(transactions.begin(), transactions.end(), t)};
  if (it != transactions.end())
    transactions.erase(it);
}

void InMemoryAccount::removeDebit(const Transaction &t) { remove(debits, t); }

void InMemoryAccount::removeCredit(const Transaction &t) { remove(credits, t); }
} // namespace sbash64::budget