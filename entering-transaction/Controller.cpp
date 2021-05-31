#include "Controller.hpp"
#include <sbash64/budget/parse.hpp>

namespace sbash64::budget::entering_transaction {
Controller::Controller(Control &control, Model &model)
    : control{control}, model{model} {
  control.attach(this);
}

void Controller::notifyThatEnterButtonHasBeenClicked() {
  model.debit(
      control.accountName(),
      Transaction{usd(control.amountUsd()), control.description(),
                  Date{control.year(), Month{control.month()}, control.day()}});
}
} // namespace sbash64::budget::entering_transaction
