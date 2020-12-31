#include "evaluate.hpp"
#include "usd.hpp"
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>
#include <sstream>
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

  void print(std::ostream &) override {}

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

  void print(std::ostream &) override {}

private:
  testcpplite::TestResult &testResult;
  bool entered{};
};

class AssertsPrints : public ExpenseRecord {
public:
  AssertsPrints(testcpplite::TestResult &testResult,
                const std::ostream &expectedStream)
      : expectedStream{expectedStream}, testResult{testResult} {}

  ~AssertsPrints() override { assertTrue(testResult, prints); }

  void enter(const LabeledExpense &) override {}

  void print(std::ostream &s) override {
    assertTrue(testResult, &expectedStream == &s);
    prints = true;
  }

private:
  const std::ostream &expectedStream;
  testcpplite::TestResult &testResult;
  bool prints{};
};

static void assertExpenseEntered(testcpplite::TestResult &result,
                                 std::string_view c,
                                 const LabeledExpense &expected) {
  std::stringstream s;
  AssertsExpenseEntered record{result, expected};
  command(record, c, s);
}

static void assertNoExpenseEntered(testcpplite::TestResult &result,
                                   std::string_view c) {
  std::stringstream s;
  AssertsNoExpenseEntered record{result};
  command(record, c, s);
}

static void assertPrints(testcpplite::TestResult &result, std::string_view c) {
  std::stringstream s;
  AssertsPrints record{result, s};
  command(record, c, s);
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

void printCommand(testcpplite::TestResult &result) {
  assertPrints(result, "print");
}
} // namespace sbash64::budget::evaluate
