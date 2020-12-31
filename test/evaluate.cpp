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

class AssertsNoExpenseEntered : public ExpenseRecord {
public:
  AssertsNoExpenseEntered(testcpplite::TestResult &testResult)
      : testResult{testResult} {}

  ~AssertsNoExpenseEntered() override { assertFalse(testResult, entered); }

  void enter(const LabeledExpense &) override { entered = true; }

private:
  testcpplite::TestResult &testResult;
  bool entered{};
};

static void assertExpenseEntered(testcpplite::TestResult &result,
                                 std::string_view c,
                                 const LabeledExpense &expected) {
  AssertsExpenseEntered record{result, expected};
  command(record, c);
}

static void assertNoExpenseEntered(testcpplite::TestResult &result,
                                   std::string_view c) {
  AssertsNoExpenseEntered record{result};
  command(record, c);
}

void expenseWithOneSubcategory(testcpplite::TestResult &result) {
  assertExpenseEntered(
      result, "Gifts Birthdays 25 Sam's 24th",
      LabeledExpense{RecursiveExpense{Category{"Gifts"},
                                      Subexpense{RecursiveExpense{
                                          Category{"Birthdays"}, 2500_cents}}},
                     "Sam's 24th"});
}

void expenseWithTwoSubcategories(testcpplite::TestResult &result) {
  assertExpenseEntered(
      result, "Gifts Birthdays Sam 25 24th",
      LabeledExpense{RecursiveExpense{Category{"Gifts"},
                                      Subexpense{RecursiveExpense{
                                          Category{"Birthdays"},
                                          Subexpense{RecursiveExpense{
                                              Category{"Sam"}, 2500_cents}}}}},
                     "24th"});
}

void expenseWithMultiWordSubcategories(testcpplite::TestResult &result) {
  assertExpenseEntered(
      result, R"(Food "Dining Out" "With Friends" 9.30 Chipotle 10/13/20)",
      LabeledExpense{
          RecursiveExpense{Category{"Food"},
                           Subexpense{RecursiveExpense{
                               Category{"Dining Out"},
                               Subexpense{RecursiveExpense{
                                   Category{"With Friends"}, 930_cents}}}}},
          "Chipotle 10/13/20"});
}

void invalidExpense(testcpplite::TestResult &result) {
  assertNoExpenseEntered(
      result, R"(Food "Dining Out" "With Friends" Chipotle 10/13/20)");
}
} // namespace sbash64::budget::evaluate
