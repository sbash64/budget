#include "collect.hpp"
#include "usd.hpp"
#include <sbash64/budget/collect.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::collect {
static void assertEqual(testcpplite::TestResult &result,
                        const ExpenseTree &expected,
                        const ExpenseTree &actual) {
  assertTrue(result, expected == actual);
}

static void
assertYieldsExpenseTree(testcpplite::TestResult &result,
                        const std::vector<RecursiveExpense> &expenses,
                        const ExpenseTree &tree) {
  assertEqual(result, tree, expenseTree(expenses));
}

void expensesToTree(testcpplite::TestResult &result) {
  assertYieldsExpenseTree(
      result,
      {RecursiveExpense{ExpenseCategory{"Gifts"},
                        Subexpense{RecursiveExpense{
                            ExpenseCategory{"Birthdays"}, 2500_cents}}},
       RecursiveExpense{ExpenseCategory{"Gifts"},
                        Subexpense{RecursiveExpense{
                            ExpenseCategory{"Birthdays"}, 1500_cents}}},
       RecursiveExpense{ExpenseCategory{"Food"},
                        Subexpense{RecursiveExpense{
                            ExpenseCategory{"Dining Out"}, 1300_cents}}},
       RecursiveExpense{ExpenseCategory{"Phone"},
                        Subexpense{RecursiveExpense{ExpenseCategory{"Verizon"},
                                                    6700_cents}}},
       RecursiveExpense{ExpenseCategory{"Food"},
                        Subexpense{RecursiveExpense{
                            ExpenseCategory{"Groceries"}, 3700_cents}}},
       RecursiveExpense{ExpenseCategory{"Food"},
                        Subexpense{RecursiveExpense{
                            ExpenseCategory{"Dining Out"}, 850_cents}}}},
      ExpenseTree{
          {{ExpenseCategory{"Food"},
            ExpenseTree{{{ExpenseCategory{"Dining Out"}, 2150_cents},
                         {ExpenseCategory{"Groceries"}, 3700_cents}}}},
           {ExpenseCategory{"Phone"},
            ExpenseTree{{{ExpenseCategory{"Verizon"}, 6700_cents}}}},
           {ExpenseCategory{"Gifts"},
            ExpenseTree{{{ExpenseCategory{"Birthdays"}, 4000_cents}}}}}});
}
} // namespace sbash64::budget::collect
