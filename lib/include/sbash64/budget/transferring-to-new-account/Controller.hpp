#ifndef SBASH64_BUDGET_TRANSFERRING_TO_NEW_ACCOUNT_CONTROLLER_HPP_
#define SBASH64_BUDGET_TRANSFERRING_TO_NEW_ACCOUNT_CONTROLLER_HPP_

#include "Model.hpp"
#include <sbash64/budget/budget.hpp>
#include <string>

namespace sbash64::budget::transferring_to_new_account {
class Control {
public:
  class Observer {
  public:
    SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Observer);
    virtual void notifyThatEnterButtonHasBeenClicked() = 0;
  };
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(Control);
  virtual void attach(Observer *) = 0;
  virtual auto amountUsd() -> std::string = 0;
  virtual auto accountName() -> std::string = 0;
};

class Controller : public Control::Observer {
public:
  Controller(Control &, Model &);
  void notifyThatEnterButtonHasBeenClicked() override;

private:
  Control &control;
  Model &model;
};
} // namespace sbash64::budget::transferring_to_new_account

#endif
