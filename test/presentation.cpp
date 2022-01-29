#include "presentation.hpp"
#include "account-stub.hpp"
#include "usd.hpp"

#include <sbash64/budget/domain.hpp>
#include <sbash64/budget/presentation.hpp>
#include <sbash64/budget/transaction.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace sbash64::budget::presentation {
namespace {
class ViewStub : public View {
public:
  void updateAccountAllocation(gsl::index accountIndex,
                               std::string_view s) override {
    allocation_ = s;
  }

  auto allocation() -> std::string { return allocation_; }

  void updateAccountBalance(gsl::index accountIndex,
                            std::string_view s) override {
    balance_ = s;
  }

  auto balance() -> std::string { return balance_; }

  void putCheckmarkNextToTransactionRow(gsl::index accountIndex,
                                        gsl::index index) override {
    checkmarkTransactionIndex_ = static_cast<int>(index);
  }

  void deleteTransactionRow(gsl::index accountIndex,
                            gsl::index index) override {
    transactionDeleted_ = static_cast<int>(index);
  }

  [[nodiscard]] auto transactionDeleted() const -> int {
    return transactionDeleted_;
  }

  [[nodiscard]] auto checkmarkTransactionIndex() const -> int {
    return checkmarkTransactionIndex_;
  }

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

  void addTransactionRow(gsl::index accountIndex, std::string_view amount,
                         std::string_view date, std::string_view description,
                         gsl::index index) override {
    transactionAddedAmount_ = amount;
    transactionAddedDate_ = date;
    transactionAddedDescription_ = description;
    transactionIndex_ = static_cast<int>(index);
  }

  [[nodiscard]] auto removedTransactionSelectionIndex() const -> int {
    return removedTransactionSelectionIndex_;
  }

  void removeTransactionRowSelection(gsl::index accountIndex,
                                     gsl::index index) override {
    removedTransactionSelectionIndex_ = static_cast<int>(index);
  }

  [[nodiscard]] auto newAccountIndex() const -> int { return newAccountIndex_; }

  void addNewAccountTable(std::string_view name, gsl::index index) override {
    newAccountIndex_ = index;
    newAccountName_ = name;
  }

  void deleteAccountTable(gsl::index) override {}

  auto newAccountName() -> std::string { return newAccountName_; }

  auto netIncome() -> std::string { return netIncome_; }

  void updateNetIncome(std::string_view s) override { netIncome_ = s; }

private:
  std::string allocation_;
  std::string balance_;
  std::string transactionAddedAmount_;
  std::string transactionAddedDate_;
  std::string transactionAddedDescription_;
  int transactionIndex_{-1};
  int checkmarkTransactionIndex_{-1};
  int transactionDeleted_{-1};
  int removedTransactionSelectionIndex_{-1};
  std::string newAccountName_;
  std::string netIncome_;
  int newAccountIndex_{-1};
};
} // namespace

static void add(AccountStub &account, ObservableTransaction &transaction,
                const ArchivableVerifiableTransaction &t) {
  account.observer()->notifyThatHasBeenAdded(transaction);
  transaction.ready(t);
}

static void test(const std::function<void(AccountPresenter &, AccountStub &,
                                          ViewStub &)> &f) {
  AccountStub account;
  ViewStub view;
  AccountPresenter presenter{account, view};
  f(presenter, account, view);
}

void formatsAccountBalance(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    account.observer()->notifyThatBalanceHasChanged(123_cents);
    assertEqual(result, "1.23", view.balance());
  });
}

void formatsAccountAllocation(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    account.observer()->notifyThatAllocationHasChanged(4680_cents);
    assertEqual(result, "46.80", view.allocation());
  });
}

void formatsTransactionAmount(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    ObservableTransactionInMemory transaction;
    add(account, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "7.89", view.transactionAddedAmount());
  });
}

void formatsTransactionDate(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    ObservableTransactionInMemory transaction;
    add(account, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "06/01/2020", view.transactionAddedDate());
  });
}

void passesDescriptionOfNewTransaction(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    ObservableTransactionInMemory transaction;
    add(account, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "chimpanzee", view.transactionAddedDescription());
  });
}

void ordersTransactionsByMostRecentDate(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    ObservableTransactionInMemory june1st2020;
    add(account, june1st2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, 0, view.transactionIndex());

    ObservableTransactionInMemory january3rd2020;
    add(account, january3rd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 3}},
         false,
         false});
    assertEqual(result, 1, view.transactionIndex());

    ObservableTransactionInMemory june4th2020;
    add(account, june4th2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 4}}, false, false});
    assertEqual(result, 0, view.transactionIndex());

    ObservableTransactionInMemory january2nd2020;
    add(account, january2nd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 2}},
         false,
         false});
    assertEqual(result, 3, view.transactionIndex());
  });
}

void ordersSameDateTransactionsByDescription(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    ObservableTransactionInMemory ape;
    add(account, ape,
        {{789_cents, "ape", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, 0, view.transactionIndex());

    ObservableTransactionInMemory chimpanzee;
    add(account, chimpanzee,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, 1, view.transactionIndex());

    ObservableTransactionInMemory baboon;
    add(account, baboon,
        {{789_cents, "baboon", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, 1, view.transactionIndex());

    ObservableTransactionInMemory dog;
    add(account, dog,
        {{789_cents, "dog", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, 3, view.transactionIndex());
  });
}

void putsCheckmarkNextToVerifiedTransaction(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    ObservableTransactionInMemory june1st2020;
    add(account, june1st2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});

    ObservableTransactionInMemory january3rd2020;
    add(account, january3rd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 3}},
         false,
         false});

    ObservableTransactionInMemory june4th2020;
    add(account, june4th2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 4}}, false, false});

    ObservableTransactionInMemory january2nd2020;
    add(account, january2nd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 2}},
         false,
         false});

    january3rd2020.verifies(
        {789_cents, "chimpanzee", Date{2020, Month::January, 3}});
    assertEqual(result, 2, view.checkmarkTransactionIndex());
  });
}

void deletesRemovedTransactionRow(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    ObservableTransactionInMemory june1st2020;
    add(account, june1st2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});

    ObservableTransactionInMemory january3rd2020;
    add(account, january3rd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 3}},
         false,
         false});

    ObservableTransactionInMemory june4th2020;
    add(account, june4th2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 4}}, false, false});

    ObservableTransactionInMemory january2nd2020;
    add(account, january2nd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 2}},
         false,
         false});

    january3rd2020.remove();
    assertEqual(result, 2, view.transactionDeleted());

    january2nd2020.remove();
    assertEqual(result, 2, view.transactionDeleted());

    june1st2020.remove();
    assertEqual(result, 1, view.transactionDeleted());

    june4th2020.remove();
    assertEqual(result, 0, view.transactionDeleted());
  });
}

void removesSelectionFromArchivedTransaction(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view) {
    ObservableTransactionInMemory june1st2020;
    add(account, june1st2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});

    ObservableTransactionInMemory january3rd2020;
    add(account, january3rd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 3}},
         false,
         false});

    ObservableTransactionInMemory june4th2020;
    add(account, june4th2020,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 4}}, false, false});

    ObservableTransactionInMemory january2nd2020;
    add(account, january2nd2020,
        {{789_cents, "chimpanzee", Date{2020, Month::January, 2}},
         false,
         false});

    january3rd2020.archive();
    assertEqual(result, 2, view.removedTransactionSelectionIndex());
  });
}

void ordersAccountsByName(testcpplite::TestResult &result) {
  ViewStub view;
  AccountStub incomeAccount;
  BudgetPresenter presenter{view, incomeAccount};
  AccountStub bob;
  presenter.notifyThatExpenseAccountHasBeenCreated(bob, "bob");
  assertEqual(result, 1, view.newAccountIndex());
  assertEqual(result, "bob", view.newAccountName());
  AccountStub dale;
  presenter.notifyThatExpenseAccountHasBeenCreated(dale, "dale");
  assertEqual(result, 2, view.newAccountIndex());
  AccountStub carl;
  presenter.notifyThatExpenseAccountHasBeenCreated(carl, "carl");
  assertEqual(result, 2, view.newAccountIndex());
  AccountStub andy;
  presenter.notifyThatExpenseAccountHasBeenCreated(andy, "andy");
  assertEqual(result, 1, view.newAccountIndex());
}

void formatsNetIncome(testcpplite::TestResult &result) {
  ViewStub view;
  AccountStub incomeAccount;
  BudgetPresenter presenter{view, incomeAccount};
  presenter.notifyThatNetIncomeHasChanged(1234_cents);
  assertEqual(result, "12.34", view.netIncome());
}
} // namespace sbash64::budget::presentation
