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
    virtual void notifyThatTransferToNewAccountButtonHasBeenPressed() = 0;
    virtual void notifyThatReduceButtonHasBeenPressed() = 0;
    virtual void notifyThatRemoveTransactionButtonHasBeenPressed() = 0;
    virtual void notifyThatVerifyTransactionButtonHasBeenPressed() = 0;
  };
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Control);
  virtual void attach(Observer *) = 0;
  virtual auto year() -> int = 0;
  virtual auto month() -> int = 0;
  virtual auto day() -> int = 0;
  virtual auto amountUsd() -> std::string = 0;
  virtual auto description() -> std::string = 0;
  virtual auto accountName() -> std::string = 0;
  virtual auto selectedTransaction() -> Transaction = 0;
  virtual auto selectedTransactionIsCredit() -> bool = 0;
};

static auto description(Control &control) -> std::string {
  return control.description();
}

static auto date(Control &control) -> Date {
  return Date{control.year(), Month{control.month()}, control.day()};
}

static auto usd(Control &control) -> USD { return usd(control.amountUsd()); }

static auto transaction(Control &control) -> Transaction {
  return {usd(control), description(control), date(control)};
}

static auto selectedAccountName(Control &control) -> std::string {
  return control.accountName();
}

static auto masterAccountIsSelected(Control &control) -> bool {
  return selectedAccountName(control) == "master";
}

static auto selectedTransaction(Control &control) -> Transaction {
  return control.selectedTransaction();
}

static auto selectedTransactionIsCredit(Control &control) -> bool {
  return control.selectedTransactionIsCredit();
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

  void notifyThatTransferToNewAccountButtonHasBeenPressed() override {
    model.transferTo(description(control), usd(control), date(control));
  }

  void notifyThatReduceButtonHasBeenPressed() override {
    model.reduce(date(control));
  }

  void notifyThatRemoveTransactionButtonHasBeenPressed() override {
    if (selectedTransactionIsCredit(control) ==
        masterAccountIsSelected(control)) {
      if (selectedTransactionIsCredit(control))
        model.removeCredit(selectedTransaction(control));
      else
        model.removeDebit(selectedAccountName(control),
                          selectedTransaction(control));
    } else if (!masterAccountIsSelected(control))
      model.removeTransfer(selectedAccountName(control), usd(control),
                           date(control));
  }

  void notifyThatVerifyTransactionButtonHasBeenPressed() override {
    if (selectedTransactionIsCredit(control) ==
        masterAccountIsSelected(control)) {
      if (selectedTransactionIsCredit(control))
        model.verifyCredit(selectedTransaction(control));
      else
        model.verifyDebit(selectedAccountName(control),
                          selectedTransaction(control));
    }
  }

private:
  Model &model;
  Control &control;
};
} // namespace sbash64::budget

#endif
