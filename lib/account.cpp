#include "account.hpp"
#include <algorithm>
#include <numeric>

namespace sbash64::budget {
void InMemoryAccount::credit(const Transaction &t) { credits.push_back(t); }

void InMemoryAccount::debit(const Transaction &t) { debits.push_back(t); }

static auto balance(const std::vector<Transaction> &credits,
                    const std::vector<Transaction> &debits) -> USD {
  const auto creditBalance{std::accumulate(
      credits.begin(), credits.end(), USD{0},
      [](USD total, const Transaction &t) { return total + t.amount; })};
  const auto debitBalance{std::accumulate(
      debits.begin(), debits.end(), USD{0},
      [](USD total, const Transaction &t) { return total + t.amount; })};
  return creditBalance - debitBalance;
}

void InMemoryAccount::print(Printer &printer) {
  std::vector<PrintableTransaction> transactions;
  for (const auto &c : credits)
    transactions.push_back(printableCredit(c));
  for (const auto &d : debits)
    transactions.push_back(printableDebit(d));
  std::sort(transactions.begin(), transactions.end(),
            [](const PrintableTransaction &a, const PrintableTransaction &b) {
              return a.transaction.date < b.transaction.date;
            });
  printer.printAccountSummary(name, balance(credits, debits), transactions);
}

InMemoryAccount::InMemoryAccount(std::string name) : name{std::move(name)} {}
} // namespace sbash64::budget