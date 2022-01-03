#include "presentation.hpp"
#include "usd.hpp"

#include <functional>
#include <sbash64/budget/presentation.hpp>
#include <sbash64/budget/transaction.hpp>

#include <string>

namespace sbash64::budget::presentation {
namespace {
class AccountViewStub : public AccountView {
public:
  auto transactionAddedAmount() -> std::string {
    return transactionAddedAmount_;
  }

  auto transactionAddedDate() -> std::string { return transactionAddedDate_; }

  void addTransaction(std::string_view amount, std::string_view date) override {
    transactionAddedAmount_ = amount;
    transactionAddedDate_ = date;
  }

private:
  std::string transactionAddedAmount_;
  std::string transactionAddedDate_;
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
  AccountViewStub view;
  AccountPresenter presenter{view};
  ObservableTransactionInMemory transaction;
  presenter.notifyThatHasBeenAdded(transaction);
  transaction.ready(
      {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
  assertEqual(result, "06/01/2020", view.transactionAddedDate());
}
} // namespace sbash64::budget::presentation
