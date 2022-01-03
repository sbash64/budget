#include "presentation.hpp"
#include "usd.hpp"

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

  void addTransaction(std::string_view amount) {
    transactionAddedAmount_ = amount;
  }

private:
  std::string transactionAddedAmount_;
};
} // namespace

void tbd(testcpplite::TestResult &result) {
  AccountViewStub view;
  AccountPresenter presenter{view};
  ObservableTransactionInMemory transaction;
  presenter.notifyThatHasBeenAdded(transaction);
  transaction.ready(
      {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
  assertEqual(result, "7.89", view.transactionAddedAmount());
}
} // namespace sbash64::budget::presentation
