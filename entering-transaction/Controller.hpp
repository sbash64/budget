#ifndef SBASH64_BUDGET_ENTERING_TRANSACTION_CONTROLLER_HPP_
#define SBASH64_BUDGET_ENTERING_TRANSACTION_CONTROLLER_HPP_

#include "Model.hpp"
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/parse.hpp>
#include <string>

namespace sbash64::budget::entering_transaction {
class Control {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatEnterButtonHasBeenClicked() = 0;
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

class Controller : public Control::Observer {
public:
  explicit Controller(Control &control, Model &model)
      : control{control}, model{model} {
    control.attach(this);
  }

  void notifyThatEnterButtonHasBeenClicked() override {
    model.submit(Transaction{
        usd(control.amountUsd()), control.description(),
        Date{control.year(), Month{control.month()}, control.day()}});
  }

private:
  Control &control;
  Model &model;
};
} // namespace sbash64::budget::entering_transaction

#endif
