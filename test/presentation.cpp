#include "presentation.hpp"
#include "usd.hpp"

#include <sbash64/budget/presentation.hpp>
#include <sbash64/budget/transaction.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace sbash64::budget::presentation {
namespace {
class AccountViewStub : public AccountView {
public:
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
  std::string transactionAddedAmount_;
  std::string transactionAddedDate_;
  std::string transactionAddedDescription_;
  int transactionIndex_{-1};
};
} // namespace

static void
test(const std::function<void(AccountPresenter &, AccountViewStub &)> &f) {
  AccountViewStub view;
  AccountPresenter presenter{view};
  f(presenter, view);
}

void formatsTransactionAmount(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    ObservableTransactionInMemory transaction;
    presenter.notifyThatHasBeenAdded(transaction);
    transaction.ready(
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "7.89", view.transactionAddedAmount());
  });
}

void formatsDate(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    ObservableTransactionInMemory transaction;
    presenter.notifyThatHasBeenAdded(transaction);
    transaction.ready(
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "06/01/2020", view.transactionAddedDate());
  });
}

void sendsDescriptionOfNewTransaction(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    ObservableTransactionInMemory transaction;
    presenter.notifyThatHasBeenAdded(transaction);
    transaction.ready(
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "chimpanzee", view.transactionAddedDescription());
  });
}

void ordersTransactionsByMostRecentDate(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &presenter, AccountViewStub &view) {
    ObservableTransactionInMemory june1st2020;
    ObservableTransactionInMemory january3rd2020;
    ObservableTransactionInMemory june4th2020;
    ObservableTransactionInMemory january2nd2020;

    presenter.notifyThatHasBeenAdded(june1st2020);
    june1st2020.ready(
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, 0, view.transactionIndex());

    presenter.notifyThatHasBeenAdded(january3rd2020);
    january3rd2020.ready(
        {{789_cents, "chimpanzee", Date{2020, Month::January, 3}},
         false,
         false});
    assertEqual(result, 1, view.transactionIndex());

    presenter.notifyThatHasBeenAdded(june4th2020);
    june4th2020.ready(
        {{789_cents, "chimpanzee", Date{2020, Month::June, 4}}, false, false});
    assertEqual(result, 0, view.transactionIndex());

    presenter.notifyThatHasBeenAdded(january2nd2020);
    january2nd2020.ready(
        {{789_cents, "chimpanzee", Date{2020, Month::January, 2}},
         false,
         false});
    assertEqual(result, 3, view.transactionIndex());
  });
}
} // namespace sbash64::budget::presentation
