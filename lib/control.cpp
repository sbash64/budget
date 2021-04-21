#include "control.hpp"

namespace sbash64::budget {
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

Controller::Controller(Model &model, Control &control)
    : model{model}, control{control} {
  control.attach(this);
}

void Controller::notifyThatTransferToButtonHasBeenPressed() {
  if (masterAccountIsSelected(control))
    return;
  model.transferTo(selectedAccountName(control), usd(control), date(control));
}

void Controller::notifyThatTransferToNewAccountButtonHasBeenPressed() {
  model.transferTo(description(control), usd(control), date(control));
}

void Controller::notifyThatReduceButtonHasBeenPressed() {
  model.reduce(date(control));
}

void Controller::notifyThatRemoveTransactionButtonHasBeenPressed() {
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

void Controller::notifyThatVerifyTransactionButtonHasBeenPressed() {
  if (selectedTransactionIsCredit(control) ==
      masterAccountIsSelected(control)) {
    if (selectedTransactionIsCredit(control))
      model.verifyCredit(selectedTransaction(control));
    else
      model.verifyDebit(selectedAccountName(control),
                        selectedTransaction(control));
  }
}

void Controller::notifyThatAddTransactionButtonHasBeenPressed() {
  if (masterAccountIsSelected(control))
    model.credit(transaction(control));
  else
    model.debit(selectedAccountName(control), transaction(control));
}
} // namespace sbash64::budget
