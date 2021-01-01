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

  void enter(const LabeledExpense &actual) override {
    assertEqual(testResult, expectedLabeledExpense, actual);
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

class AssertsPrinted : public ExpenseRecord {
public:
  AssertsPrinted(testcpplite::TestResult &testResult,
                 const std::ostream &expectedStream)
      : expectedStream{expectedStream}, testResult{testResult} {}

  ~AssertsPrinted() override { assertTrue(testResult, printed); }

  void enter(const LabeledExpense &) override {}

  void print(std::ostream &actual) override {
    assertTrue(testResult, &expectedStream == &actual);
    printed = true;
  }

private:
  const std::ostream &expectedStream;
  testcpplite::TestResult &testResult;
  bool printed{};
};

class ExpenseRecordStub : public ExpenseRecord {
public:
  void enter(const LabeledExpense &) override {}

  void print(std::ostream &) override {}
};

static void assertExpenseEntered(testcpplite::TestResult &result,
                                 std::string_view input,
                                 const LabeledExpense &expected) {
  std::stringstream output;
  AssertsExpenseEntered record{result, expected};
  command(record, input, output);
}

static void assertNoExpenseEntered(testcpplite::TestResult &result,
                                   std::string_view input) {
  std::stringstream output;
  AssertsNoExpenseEntered record{result};
  command(record, input, output);
}

static void assertExpenseRecordPrinted(testcpplite::TestResult &result,
                                       std::string_view input) {
  std::stringstream output;
  AssertsPrinted record{result, output};
  command(record, input, output);
}

static void assertPrints(testcpplite::TestResult &result,
                         std::string_view input, std::string_view expected) {
  std::stringstream output;
  ExpenseRecordStub record;
  command(record, input, output);
  assertEqual(result, std::string{expected}, output.str());
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
  assertExpenseRecordPrinted(result, "print");
}

void expenseShouldPrintExpense(testcpplite::TestResult &result) {
  assertPrints(result, "Gifts Birthdays 25 Sam's 24th",
               "Gifts::Birthdays: $25.00 - Sam's 24th");
}

void invalidExpenseShouldPrintMessage(testcpplite::TestResult &result) {
  assertPrints(result, "Gifts Birthdays Sam 24th",
               "No expense entered because no amount found.");
}
} // namespace sbash64::budget::evaluate
