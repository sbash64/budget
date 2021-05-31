#include "Controller.hpp"
#include "../usd.hpp"
#include "sbash64/testcpplite/testcpplite.hpp"
#include <sbash64/budget/entering-transaction/Controller.hpp>
#include <utility>

namespace sbash64::budget::entering_transaction::controller {
namespace {
class ControlStub : public Control {
public:
  void notifyThatEnterButtonHasBeenClicked() {
    observer->notifyThatEnterButtonHasBeenClicked();
  }

  void attach(Observer *a) override { observer = a; }

  auto year() -> int override { return year_; }

  void setYear(int a) { year_ = a; }

  auto month() -> int override { return month_; }

  void setMonth(int a) { month_ = a; }

  auto day() -> int override { return day_; }

  void setDay(int a) { day_ = a; }

  auto amountUsd() -> std::string override { return amountUsd_; }

  void setAmountUsd(std::string a) { amountUsd_ = std::move(a); }

  auto description() -> std::string override { return description_; }

  void setDescription(std::string a) { description_ = std::move(a); }

  auto accountName() -> std::string override { return accountName_; }

  void setAccountName(std::string a) { accountName_ = std::move(a); }

  void setDebit(bool b) { debit_ = b; }

  auto debit() -> bool { return debit_; }

private:
  Observer *observer{};
  int year_{};
  int month_{};
  int day_{};
  std::string amountUsd_;
  std::string description_;
  std::string accountName_;
  bool debit_{};
};

class ModelStub : public Model {
public:
  auto debit() -> Transaction { return debit_; }

  auto credit() -> Transaction { return credit_; }

  auto accountName() -> std::string { return accountName_; }

  void debit(std::string_view accountName, const Transaction &t) override {
    accountName_ = accountName;
    debit_ = t;
  }

  void credit(const Transaction &t) { credit_ = t; }

private:
  Transaction debit_;
  Transaction credit_;
  std::string accountName_;
};
} // namespace

void shouldTranslateControlDataToDebit(
    sbash64::testcpplite::TestResult &result) {
  ControlStub control;
  ModelStub model;
  Controller controller{control, model};
  control.setYear(1);
  control.setMonth(2);
  control.setDay(3);
  control.setAmountUsd("4.56");
  control.setDescription("lemon");
  control.setAccountName("lime");
  control.setDebit(true);
  control.notifyThatEnterButtonHasBeenClicked();
  assertEqual(result, Transaction{456_cents, "lemon", Date{1, Month{2}, 3}},
              model.debit());
  assertEqual(result, "lime", model.accountName());
}

void shouldTranslateControlDataToCredit(
    sbash64::testcpplite::TestResult &result) {
  ControlStub control;
  ModelStub model;
  Controller controller{control, model};
  control.setYear(1);
  control.setMonth(2);
  control.setDay(3);
  control.setAmountUsd("4.56");
  control.setDescription("lemon");
  control.setDebit(false);
  control.notifyThatEnterButtonHasBeenClicked();
  assertEqual(result, Transaction{456_cents, "lemon", Date{1, Month{2}, 3}},
              model.credit());
}
} // namespace sbash64::budget::entering_transaction::controller
