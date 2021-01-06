#include "evaluate.hpp"
#include "usd.hpp"
#include "view-stub.hpp"
#include <functional>
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
#include <string_view>

namespace sbash64::budget::evaluate {
namespace {
class ModelStub : public Model {
public:
  auto debitedAccountName() -> std::string { return debitedAccountName_; }

  auto debitedTransaction() -> Transaction { return debitedTransaction_; }

  auto creditedTransaction() -> Transaction { return creditedTransaction_; }

  void debit(std::string_view accountName,
             const Transaction &transaction) override {
    debitedAccountName_ = accountName;
    debitedTransaction_ = transaction;
  }

  void credit(const Transaction &transaction) override {
    creditedTransaction_ = transaction;
  }

  void transferTo(std::string_view accountName, USD amount,
                  Date date) override {
    transferredAmount_ = amount;
    accountNameTransferredTo_ = accountName;
    transferDate_ = date;
  }

  void print(View &p) override { printer_ = &p; }

  auto view() -> const View * { return printer_; }

  void save(PersistentMemory &p) override { persistentMemory_ = &p; }

  auto persistentMemory() -> const PersistentMemory * {
    return persistentMemory_;
  }

  auto transferredAmount() -> USD { return transferredAmount_; }

  auto accountNameTransferredTo() -> std::string {
    return accountNameTransferredTo_;
  }

  auto transferDate() -> Date { return transferDate_; }

private:
  Transaction debitedTransaction_;
  Transaction creditedTransaction_;
  std::string debitedAccountName_;
  const View *printer_{};
  const PersistentMemory *persistentMemory_{};
  USD transferredAmount_{};
  std::string accountNameTransferredTo_;
  Date transferDate_{};
};

class PersistentMemoryStub : public PersistentMemory {};
} // namespace

static void
testController(const std::function<void(Controller &, ModelStub &, ViewStub &,
                                        PersistentMemoryStub &)> &f) {
  Controller controller;
  ModelStub model;
  ViewStub view;
  PersistentMemoryStub persistentMemory;
  f(controller, model, view, persistentMemory);
}

static void assertPrints(testcpplite::TestResult &result,
                         std::string_view input) {
  testController([&](Controller &controller, ModelStub &model, ViewStub &view,
                     PersistentMemoryStub &persistentMemory) {
    command(controller, model, view, persistentMemory, input);
    assertEqual(result, &view, model.view());
  });
}

static void assertSaves(testcpplite::TestResult &result,
                        std::string_view input) {
  testController([&](Controller &controller, ModelStub &model, ViewStub &view,
                     PersistentMemoryStub &persistentMemory) {
    command(controller, model, view, persistentMemory, input);
    assertEqual(result, &persistentMemory, model.persistentMemory());
  });
}

static void assertDebitsAccount(testcpplite::TestResult &result,
                                const std::vector<std::string> &input,
                                const std::string &expectedAccountName,
                                const Transaction &expectedTransaction) {
  testController([&](Controller &controller, ModelStub &model, ViewStub &view,
                     PersistentMemoryStub &persistentMemory) {
    for (const auto &x : input)
      command(controller, model, view, persistentMemory, x);
    assertEqual(result, expectedAccountName, model.debitedAccountName());
    assertEqual(result, expectedTransaction, model.debitedTransaction());
  });
}

static void assertCreditsAccount(testcpplite::TestResult &result,
                                 const std::vector<std::string> &input,
                                 const Transaction &expectedTransaction) {
  testController([&](Controller &controller, ModelStub &bank, ViewStub &view,
                     PersistentMemoryStub &persistentMemory) {
    for (const auto &x : input)
      command(controller, bank, view, persistentMemory, x);
    assertEqual(result, expectedTransaction, bank.creditedTransaction());
  });
}

static void assertTransfersToAccount(testcpplite::TestResult &result,
                                     const std::vector<std::string> &input,
                                     USD expectedAmount,
                                     std::string_view expectedAccountName,
                                     const Date &expectedDate) {
  testController([&](Controller &controller, ModelStub &model, ViewStub &view,
                     PersistentMemoryStub &persistentMemory) {
    for (const auto &x : input)
      command(controller, model, view, persistentMemory, x);
    assertEqual(result, expectedAmount, model.transferredAmount());
    assertEqual(result, std::string{expectedAccountName},
                model.accountNameTransferredTo());
    assertEqual(result, expectedDate, model.transferDate());
  });
}

void print(testcpplite::TestResult &result) { assertPrints(result, "print"); }

void debit(testcpplite::TestResult &result) {
  assertDebitsAccount(
      result, {"debit Gifts 25", "12 27 20", "Sam's 24th"}, "Gifts",
      Transaction{2500_cents, "Sam's 24th", Date{2020, Month::December, 27}});
}

void credit(testcpplite::TestResult &result) {
  assertCreditsAccount(
      result, {"credit 2134.35", "11 22 19", "btnrh"},
      Transaction{213435_cents, "btnrh", Date{2019, Month::November, 22}});
}

void transferTo(testcpplite::TestResult &result) {
  assertTransfersToAccount(result, {"transferto Groceries 50", "6 3 21"},
                           5000_cents, "Groceries", Date{2021, Month::June, 3});
}

void save(testcpplite::TestResult &result) { assertSaves(result, "save"); }
} // namespace sbash64::budget::evaluate
