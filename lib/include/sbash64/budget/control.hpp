#ifndef SBASH64_BUDGET_CONTROL_HPP_
#define SBASH64_BUDGET_CONTROL_HPP_

#include "budget.hpp"

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
    virtual void notifyThatRemoveAccountButtonHasBeenPressed() = 0;
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

class Controller : Control::Observer {
public:
  explicit Controller(Budget &, Control &);
  void notifyThatAddTransactionButtonHasBeenPressed() override;
  void notifyThatTransferToButtonHasBeenPressed() override;
  void notifyThatTransferToNewAccountButtonHasBeenPressed() override;
  void notifyThatReduceButtonHasBeenPressed() override;
  void notifyThatRemoveTransactionButtonHasBeenPressed() override;
  void notifyThatVerifyTransactionButtonHasBeenPressed() override;
  void notifyThatRemoveAccountButtonHasBeenPressed() override;

private:
  Budget &model;
  Control &control;
};
} // namespace sbash64::budget

#endif
