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
  void setAccountName(gsl::index, std::string_view) override {}

  void updateAccountAllocation(gsl::index accountIndex,
                               std::string_view s) override {
    accountIndex_ = static_cast<int>(accountIndex);
    allocation_ = s;
  }

  auto allocation() -> std::string { return allocation_; }

  void updateAccountBalance(gsl::index accountIndex,
                            std::string_view s) override {
    accountIndex_ = static_cast<int>(accountIndex);
    balance_ = s;
  }

  auto balance() -> std::string { return balance_; }

  void putCheckmarkNextToTransactionRow(gsl::index accountIndex,
                                        gsl::index index) override {
    accountIndex_ = static_cast<int>(accountIndex);
    checkmarkTransactionIndex_ = static_cast<int>(index);
  }

  void deleteTransactionRow(gsl::index accountIndex,
                            gsl::index index) override {
    accountIndex_ = static_cast<int>(accountIndex);
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
    accountIndex_ = static_cast<int>(accountIndex);
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
    accountIndex_ = static_cast<int>(accountIndex);
    removedTransactionSelectionIndex_ = static_cast<int>(index);
  }

  [[nodiscard]] auto newAccountIndex() const -> int { return newAccountIndex_; }

  void addNewAccountTable(std::string_view name, gsl::index index) override {
    newAccountIndex_ = static_cast<int>(index);
    newAccountName_ = name;
  }

  void deleteAccountTable(gsl::index) override {}

  auto newAccountName() -> std::string { return newAccountName_; }

  auto netIncome() -> std::string { return netIncome_; }

  void updateNetIncome(std::string_view s) override { netIncome_ = s; }

  [[nodiscard]] auto accountIndex() const -> int { return accountIndex_; }

  [[nodiscard]] auto markedAsSaved() const -> bool { return markedAsSaved_; }

  void markAsSaved() override { markedAsSaved_ = true; }

  [[nodiscard]] auto markedAsUnsaved() const -> bool {
    return markedAsUnsaved_;
  }

  void markAsUnsaved() override { markedAsUnsaved_ = true; }

  void reorderAccountIndex(gsl::index from, gsl::index to) override {
    reorderedAccountFromIndex = from;
    reorderedAccountToIndex = to;
  }

  gsl::index reorderedAccountFromIndex{-1};
  gsl::index reorderedAccountToIndex{-1};

private:
  std::string allocation_;
  std::string balance_;
  std::string transactionAddedAmount_;
  std::string transactionAddedDate_;
  std::string transactionAddedDescription_;
  std::string newAccountName_;
  std::string netIncome_;
  int transactionIndex_{-1};
  int accountIndex_{-1};
  int checkmarkTransactionIndex_{-1};
  int transactionDeleted_{-1};
  int removedTransactionSelectionIndex_{-1};
  int newAccountIndex_{-1};
  bool markedAsSaved_{};
  bool markedAsUnsaved_{};
};

class AccountPresenterParentStub : public AccountPresenter::Parent {
public:
  void remove(const AccountPresenter *) override {}
  void reorder(const AccountPresenter *, std::string_view) override {}
  auto index(const AccountPresenter *) -> gsl::index override { return index_; }
  gsl::index index_{-1};
};
} // namespace

static void add(AccountStub &account, ObservableTransaction &transaction,
                const ArchivableVerifiableTransaction &t) {
  account.observer()->notifyThatHasBeenAdded(transaction);
  transaction.initialize(t);
  if (t.verified)
    transaction.verifies(t);
  if (t.archived)
    transaction.archive();
}

static void
test(const std::function<void(AccountPresenter &, AccountStub &, ViewStub &,
                              AccountPresenterParentStub &)> &f) {
  AccountStub account;
  ViewStub view;
  std::set<View *> views{&view};
  AccountPresenterParentStub parent;
  AccountPresenter presenter{account, views, "", parent};
  f(presenter, account, view, parent);
}

void formatsAccountBalance(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &parent) {
    parent.index_ = 1;
    account.observer()->notifyThatBalanceHasChanged(123_cents);
    assertEqual(result, "1.23", view.balance());
    assertEqual(result, 1, view.accountIndex());
  });
}

void formatsAccountAllocation(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &parent) {
    parent.index_ = 42;
    account.observer()->notifyThatAllocationHasChanged(4680_cents);
    assertEqual(result, "46.80", view.allocation());
    assertEqual(result, 42, view.accountIndex());
  });
}

void formatsTransactionAmount(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &parent) {
    parent.index_ = 3;
    ObservableTransactionInMemory transaction;
    add(account, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "7.89", view.transactionAddedAmount());
    assertEqual(result, 3, view.accountIndex());
  });
}

void formatsTransactionDate(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &) {
    ObservableTransactionInMemory transaction;
    add(account, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "06/01/2020", view.transactionAddedDate());
  });
}

void passesDescriptionOfNewTransaction(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &) {
    ObservableTransactionInMemory transaction;
    add(account, transaction,
        {{789_cents, "chimpanzee", Date{2020, Month::June, 1}}, false, false});
    assertEqual(result, "chimpanzee", view.transactionAddedDescription());
  });
}

void ordersTransactionsByMostRecentDate(testcpplite::TestResult &result) {
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &) {
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
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &) {
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
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &) {
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
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &) {
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
  test([&result](AccountPresenter &, AccountStub &account, ViewStub &view,
                 AccountPresenterParentStub &) {
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

void reordersAccountsByName(testcpplite::TestResult &result) {
  ViewStub view;
  AccountStub incomeAccount;
  BudgetPresenter presenter{incomeAccount};
  presenter.add(&view);
  AccountStub bob;
  presenter.notifyThatExpenseAccountHasBeenCreated(bob, "bob");
  AccountStub daleOrAdam;
  presenter.notifyThatExpenseAccountHasBeenCreated(daleOrAdam, "dale");
  AccountStub carlOrPeggy;
  presenter.notifyThatExpenseAccountHasBeenCreated(carlOrPeggy, "carl");
  carlOrPeggy.observer()->notifyThatNameHasChanged("peggy");
  assertEqual(result, 2L, view.reorderedAccountFromIndex);
  assertEqual(result, 3L, view.reorderedAccountToIndex);
  daleOrAdam.observer()->notifyThatNameHasChanged("adam");
  assertEqual(result, 2L, view.reorderedAccountFromIndex);
  assertEqual(result, 1L, view.reorderedAccountToIndex);
}

void ordersAccountsByName(testcpplite::TestResult &result) {
  ViewStub view;
  AccountStub incomeAccount;
  BudgetPresenter presenter{incomeAccount};
  presenter.add(&view);
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
  BudgetPresenter presenter{incomeAccount};
  presenter.add(&view);
  presenter.notifyThatNetIncomeHasChanged(1234_cents);
  assertEqual(result, "12.34", view.netIncome());
}

void marksAsSaved(testcpplite::TestResult &result) {
  ViewStub view;
  AccountStub incomeAccount;
  BudgetPresenter presenter{incomeAccount};
  presenter.add(&view);
  presenter.notifyThatHasBeenSaved();
  assertTrue(result, view.markedAsSaved());
}

void marksAsUnsaved(testcpplite::TestResult &result) {
  ViewStub view;
  AccountStub incomeAccount;
  BudgetPresenter presenter{incomeAccount};
  presenter.add(&view);
  presenter.notifyThatHasUnsavedChanges();
  assertTrue(result, view.markedAsUnsaved());
}
} // namespace sbash64::budget::presentation
