#include "presentation.hpp"
#include "usd.hpp"

#include <sbash64/budget/domain.hpp>
#include <sbash64/budget/presentation.hpp>
#include <sbash64/budget/transaction.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace sbash64::budget::presentation {
namespace {
class AccountViewStub : public AccountView {
public:
  void updateAllocation(std::string_view s) override { allocation_ = s; }

  auto allocation() -> std::string { return allocation_; }

  void updateBalance(std::string_view s) override { balance_ = s; }

  auto balance() -> std::string { return balance_; }

  [[nodiscard]] auto transactionIndex() const -> int {
    return transactionIndex_;
  }

  auto transactionAddedAmount() -> std::string {
    return transactionAddedAmount_;
  }

  auto transactionAddedDate() -> std::string { return transactionAddedDate_; }

  auto transactionAddedDescription() -> std::string {
    return transactionAddedDescription_;
  }

  void addTransaction(std::string_view amount, std::string_view date,
                      std::string_view description, int index) override {
    transactionAddedAmount_ = amount;
    transactionAddedDate_ = date;
    transactionAddedDescription_ = description;
    transactionIndex_ = index;
  }

private:
  std::string allocation_;
  std::string balance_;
  std::string transactionAddedAmount_;
  std::string transactionAddedDate_;
  std::string transactionAddedDescription_;
  int transactionIndex_{-1};
};
} // namespace

static void add(AccountPresenter &presenter, ObservableTransaction &transaction,
                const ArchivableVerifiableTransaction &t) {
  presenter.notifyThatHasBeenAdded(transaction);
  transaction.ready(t);
}

static void
test(const std::function<void(AccountPresenter &, AccountViewStub &)> &f) {
  AccountViewStub view;
  AccountPresenter presenter{view};
  f(presenter, view);
}

void formatsBalance(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    presenter.notifyThatBalanceHasChanged(123_cents);
    assertEqual(result, "1.23", view.balance());
  });
}

void formatsAllocation(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    presenter.notifyThatAllocationHasChanged(4680_cents);
    assertEqual(result, "46.80", view.allocation());
  });
}

void formatsTransactionAmount(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    ObservableTransactionInMemory transaction;
    add(presenter, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "7.89", view.transactionAddedAmount());
  });
}

void formatsDate(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    ObservableTransactionInMemory transaction;
    add(presenter, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "06/01/2020", view.transactionAddedDate());
  });
}

void sendsDescriptionOfNewTransaction(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    ObservableTransactionInMemory transaction;
    add(presenter, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "chimpanzee", view.transactionAddedDescription());
  });
}

void ordersTransactionsByMostRecentDate(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    ObservableTransactionInMemory june1st2020;
    add(presenter, june1st2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, 0, view.transactionIndex());

    ObservableTransactionInMemory january3rd2020;
    add(presenter, january3rd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 3}},
         false,
         false});
    assertEqual(result, 1, view.transactionIndex());

    ObservableTransactionInMemory june4th2020;
    add(presenter, june4th2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 4}}, false, false});
    assertEqual(result, 0, view.transactionIndex());

    ObservableTransactionInMemory january2nd2020;
    add(presenter, january2nd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 2}},
         false,
         false});
    assertEqual(result, 3, view.transactionIndex());
  });
}
} // namespace sbash64::budget::presentation
