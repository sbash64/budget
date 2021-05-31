#include "Controller.hpp"
#include <sbash64/budget/parse.hpp>

namespace sbash64::budget::entering_transaction {
Controller::Controller(Control &control, Model &model)
    : control{control}, model{model} {
  control.attach(this);
}

static auto transaction(Control &control) -> Transaction {
  return Transaction{
      usd(control.amountUsd()), control.description(),
      Date{control.year(), Month{control.month()}, control.day()}};
}

void Controller::notifyThatEnterButtonHasBeenClicked() {
  control.debit() ? model.debit(control.accountName(), transaction(control))
                  : model.credit(transaction(control));
}
} // namespace sbash64::budget::entering_transaction
