#include "evaluate.hpp"
#include "usd.hpp"
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <string_view>

namespace sbash64::budget::evaluate {
static void assertEqual(testcpplite::TestResult &result,
                        const LabeledExpense &expected,
                        const LabeledExpense &actual) {
  assertTrue(result, expected == actual);
}

class AssertsExpenseEntered : public ExpenseRecord {
public:
  AssertsExpenseEntered(testcpplite::TestResult &testResult,
                        const LabeledExpense &expectedLabeledExpense)
      : expectedLabeledExpense{expectedLabeledExpense}, testResult{testResult} {
  }

  ~AssertsExpenseEntered() override { assertTrue(testResult, entered); }

  void enter(const LabeledExpense &e) override {
    assertEqual(testResult, expectedLabeledExpense, e);
    entered = true;
  }

private:
  const LabeledExpense &expectedLabeledExpense;
  testcpplite::TestResult &testResult;
  bool entered{};
};

static void assertExpenseEntered(testcpplite::TestResult &result,
                                 std::string_view c,
                                 const LabeledExpense &expected) {
  AssertsExpenseEntered record{result, expected};
  command(record, c);
}

void enteringExpense(testcpplite::TestResult &result) {
  assertExpenseEntered(
      result, "Gifts Birthdays 25 Sam's 24th",
      LabeledExpense{
          RecursiveExpense{ExpenseCategory{"Gifts"},
                           Subexpense{RecursiveExpense{
                               ExpenseCategory{"Birthdays"}, 2500_cents}}},
          "Sam's 24th"});
}
} // namespace sbash64::budget::evaluate
