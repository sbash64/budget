#ifndef SBASH64_BUDGET_CONTROL_HPP_
#define SBASH64_BUDGET_CONTROL_HPP_

#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/parse.hpp>

namespace sbash64::budget {
class Control {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatAddTransactionButtonHasBeenPressed() = 0;
    virtual void notifyThatTransferToButtonHasBeenPressed() = 0;
  };
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Control);
  virtual void attach(Observer *) = 0;
  virtual auto year() -> int = 0;
  virtual auto month() -> int = 0;
  virtual auto day() -> int = 0;
  virtual auto amountUsd() -> std::string = 0;
  virtual auto description() -> std::string = 0;
  virtual auto accountName() -> std::string = 0;
};

static auto date(Control &control) -> Date {
  return Date{control.year(), Month{control.month()}, control.day()};
}

static auto usd(Control &control) -> USD { return usd(control.amountUsd()); }

static auto transaction(Control &control) -> Transaction {
  return {usd(control), control.description(), date(control)};
}

static auto selectedAccountName(Control &control) -> std::string {
  return control.accountName();
}

static auto masterAccountIsSelected(Control &control) -> bool {
  return selectedAccountName(control) == "master";
}

class Controller : Control::Observer {
public:
  explicit Controller(Model &model, Control &control)
      : model{model}, control{control} {
    control.attach(this);
  }

  void notifyThatAddTransactionButtonHasBeenPressed() override {
    if (masterAccountIsSelected(control))
      model.credit(transaction(control));
    else
      model.debit(selectedAccountName(control), transaction(control));
  }

  void notifyThatTransferToButtonHasBeenPressed() override {
    if (masterAccountIsSelected(control))
      return;
    model.transferTo(selectedAccountName(control), usd(control), date(control));
  }

private:
  Model &model;
  Control &control;
};
} // namespace sbash64::budget

#endif
