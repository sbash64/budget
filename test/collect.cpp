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
      {RecursiveExpense{
           Category{"Gifts"},
           Subexpense{RecursiveExpense{Category{"Birthdays"}, 2500_cents}}},
       RecursiveExpense{
           Category{"Gifts"},
           Subexpense{RecursiveExpense{Category{"Birthdays"}, 1500_cents}}},
       RecursiveExpense{
           Category{"Food"},
           Subexpense{RecursiveExpense{Category{"Dining Out"}, 1300_cents}}},
       RecursiveExpense{
           Category{"Phone"},
           Subexpense{RecursiveExpense{Category{"Verizon"}, 6700_cents}}},
       RecursiveExpense{
           Category{"Food"},
           Subexpense{RecursiveExpense{Category{"Groceries"}, 3700_cents}}},
       RecursiveExpense{
           Category{"Food"},
           Subexpense{RecursiveExpense{Category{"Dining Out"}, 850_cents}}}},
      ExpenseTree{{{Category{"Food"},
                    ExpenseTree{{{Category{"Dining Out"}, 2150_cents},
                                 {Category{"Groceries"}, 3700_cents}}}},
                   {Category{"Phone"},
                    ExpenseTree{{{Category{"Verizon"}, 6700_cents}}}},
                   {Category{"Gifts"},
                    ExpenseTree{{{Category{"Birthdays"}, 4000_cents}}}}}});
}
} // namespace sbash64::budget::collect
