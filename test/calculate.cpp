#include "calculate.hpp"
#include "usd.hpp"
#include <algorithm>
#include <sbash64/budget/calculate.hpp>

namespace sbash64::budget::calculate {
void surplusHavingNoIncomeNorExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, surplus(Income{0_cents}, Expenses{}));
}

void surplusHavingIncomeButNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents, surplus(Income{1_cents}, Expenses{}));
}

void surplusHavingOneExpense(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents,
              surplus(Income{3_cents}, Expenses{{Expense{2_cents}}}));
}

void surplusHavingTwoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 6_cents,
              surplus(Income{11_cents},
                      Expenses{{Expense{2_cents}, Expense{3_cents}}}));
}

void surplusHavingMultipleExpenses(testcpplite::TestResult &result) {
  assertEqual(
      result, 1000_cents,
      surplus(
          Income{10000_cents},
          ExpenseTree{{{Category{"Food"},
                        ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                                     {Category{"Groceries"}, 300_cents}}}},
                       {Category{"Phone"},
                        ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                     {Category{"Light"}, 500_cents}}}},
                       {Category{"Health"},
                        ExpenseTree{{{Category{"Gym"}, 600_cents},
                                     {Category{"Other"}, 700_cents}}}},
                       {Category{"Gifts"},
                        ExpenseTree{{{Category{"Christmas"}, 800_cents},
                                     {Category{"Birthdays"}, 900_cents},
                                     {Category{"Anniversary"}, 1000_cents}}}},
                       {Category{"Entertainment"}, 1100_cents},
                       {Category{"Car Loans"},
                        ExpenseTree{{{Category{"Honda"}, 1200_cents},
                                     {Category{"Ford"}, 1300_cents}}}}}}));
}

void surplusAfterUpdate(testcpplite::TestResult &result) {
  assertEqual(
      result, 800_cents,
      surplus(
          Income{10000_cents},
          ExpenseTree{{{Category{"Food"},
                        ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                                     {Category{"Groceries"}, 300_cents}}}},
                       {Category{"Phone"},
                        ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                     {Category{"Light"}, 500_cents}}}},
                       {Category{"Health"},
                        ExpenseTree{{{Category{"Gym"}, 600_cents},
                                     {Category{"Other"}, 700_cents}}}},
                       {Category{"Gifts"},
                        ExpenseTree{{{Category{"Christmas"}, 800_cents},
                                     {Category{"Birthdays"}, 900_cents},
                                     {Category{"Anniversary"}, 1000_cents}}}},
                       {Category{"Entertainment"}, 1100_cents},
                       {Category{"Car Loans"},
                        ExpenseTree{{{Category{"Honda"}, 1200_cents},
                                     {Category{"Ford"}, 1300_cents}}}}}},
          RecursiveExpense{Category{"Gifts"},
                           Subexpense{RecursiveExpense{Category{"Birthdays"},
                                                       1100_cents}}}));
}

void categoryTotalHavingNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, total(Category{"miscellaneous"}, Expenses{}));
}

void categoryTotalHavingOneUnrelatedExpense(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents,
              total(Category{"miscellaneous"},
                    Expenses{{Expense{1_cents, Category{"groceries"}}}}));
}

void categoryTotalHavingOneExpense(testcpplite::TestResult &result) {
  assertEqual(result, 1_cents,
              total(Category{"miscellaneous"},
                    Expenses{{Expense{1_cents, Category{"miscellaneous"}}}}));
}

void categoryTotalHavingMultipleExpenseTrees(testcpplite::TestResult &result) {
  assertEqual(
      result, 2700_cents,
      total(ExpenseTree{{{Category{"Food"},
                          ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                                       {Category{"Groceries"}, 300_cents}}}},
                         {Category{"Phone"},
                          ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                       {Category{"Light"}, 500_cents}}}},
                         {Category{"Health"},
                          ExpenseTree{{{Category{"Gym"}, 600_cents},
                                       {Category{"Other"}, 700_cents}}}},
                         {Category{"Gifts"},
                          ExpenseTree{{{Category{"Christmas"}, 800_cents},
                                       {Category{"Birthdays"}, 900_cents},
                                       {Category{"Anniversary"}, 1000_cents}}}},
                         {Category{"Entertainment"}, 1100_cents},
                         {Category{"Car Loans"},
                          ExpenseTree{{{Category{"Honda"}, 1200_cents},
                                       {Category{"Ford"}, 1300_cents}}}}}},
            Category{"Gifts"}));
}

void categoryTotalHavingMultipleExpenseTrees2(testcpplite::TestResult &result) {
  assertEqual(
      result, 1100_cents,
      total(ExpenseTree{{{Category{"Food"},
                          ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                                       {Category{"Groceries"}, 300_cents}}}},
                         {Category{"Phone"},
                          ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                       {Category{"Light"}, 500_cents}}}},
                         {Category{"Health"},
                          ExpenseTree{{{Category{"Gym"}, 600_cents},
                                       {Category{"Other"}, 700_cents}}}},
                         {Category{"Gifts"},
                          ExpenseTree{{{Category{"Christmas"}, 800_cents},
                                       {Category{"Birthdays"}, 900_cents},
                                       {Category{"Anniversary"}, 1000_cents}}}},
                         {Category{"Entertainment"}, 1100_cents},
                         {Category{"Car Loans"},
                          ExpenseTree{{{Category{"Honda"}, 1200_cents},
                                       {Category{"Ford"}, 1300_cents}}}}}},
            Category{"Entertainment"}));
}

void categoryTotalHavingMultipleExpenseTrees3(testcpplite::TestResult &result) {
  assertEqual(
      result, 400_cents,
      total(ExpenseTree{{{Category{"Food"},
                          ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                                       {Category{"Groceries"}, 300_cents}}}},
                         {Category{"Phone"},
                          ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                       {Category{"Light"}, 500_cents}}}},
                         {Category{"Health"},
                          ExpenseTree{{{Category{"Gym"}, 600_cents},
                                       {Category{"Other"}, 700_cents}}}},
                         {Category{"Gifts"},
                          ExpenseTree{{{Category{"Christmas"}, 800_cents},
                                       {Category{"Birthdays"}, 900_cents},
                                       {Category{"Anniversary"}, 1000_cents}}}},
                         {Category{"Entertainment"}, 1100_cents},
                         {Category{"Car Loans"},
                          ExpenseTree{{{Category{"Honda"}, 1200_cents},
                                       {Category{"Ford"}, 1300_cents}}}}}},
            RecursiveCategory{Category{"Phone"}, Subcategory{RecursiveCategory{
                                                     Category{"Verizon"}}}}));
}

void categoryTotalHavingMultipleExpenseTrees4(testcpplite::TestResult &result) {
  assertEqual(
      result, 0_cents,
      total(ExpenseTree{{{Category{"Food"},
                          ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                                       {Category{"Groceries"}, 300_cents}}}},
                         {Category{"Phone"},
                          ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                       {Category{"Light"}, 500_cents}}}},
                         {Category{"Health"},
                          ExpenseTree{{{Category{"Gym"}, 600_cents},
                                       {Category{"Other"}, 700_cents}}}},
                         {Category{"Gifts"},
                          ExpenseTree{{{Category{"Christmas"}, 800_cents},
                                       {Category{"Birthdays"}, 900_cents},
                                       {Category{"Anniversary"}, 1000_cents}}}},
                         {Category{"Entertainment"}, 1100_cents},
                         {Category{"Car Loans"},
                          ExpenseTree{{{Category{"Honda"}, 1200_cents},
                                       {Category{"Ford"}, 1300_cents}}}}}},
            RecursiveCategory{Category{"Health"}, Subcategory{RecursiveCategory{
                                                      Category{"Vitamins"}}}}));
}

void categoryTotalHavingMultipleExpenseTrees5(testcpplite::TestResult &result) {
  assertEqual(
      result, 0_cents,
      total(ExpenseTree{{{Category{"Food"},
                          ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                                       {Category{"Groceries"}, 300_cents}}}},
                         {Category{"Phone"},
                          ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                       {Category{"Light"}, 500_cents}}}},
                         {Category{"Health"},
                          ExpenseTree{{{Category{"Gym"}, 600_cents},
                                       {Category{"Other"}, 700_cents}}}},
                         {Category{"Gifts"},
                          ExpenseTree{{{Category{"Christmas"}, 800_cents},
                                       {Category{"Birthdays"}, 900_cents},
                                       {Category{"Anniversary"}, 1000_cents}}}},
                         {Category{"Entertainment"}, 1100_cents},
                         {Category{"Car Loans"},
                          ExpenseTree{{{Category{"Honda"}, 1200_cents},
                                       {Category{"Ford"}, 1300_cents}}}}}},
            RecursiveCategory{
                Category{"Vacation"},
                Subcategory{RecursiveCategory{Category{"Car Rental"}}}}));
}

void categoryTotalHavingMultipleExpenseTrees6(testcpplite::TestResult &result) {
  assertEqual(
      result, 0_cents,
      total(ExpenseTree{{{Category{"Food"},
                          ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                                       {Category{"Groceries"}, 300_cents}}}},
                         {Category{"Phone"},
                          ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                       {Category{"Light"}, 500_cents}}}},
                         {Category{"Health"},
                          ExpenseTree{{{Category{"Gym"}, 600_cents},
                                       {Category{"Other"}, 700_cents}}}},
                         {Category{"Gifts"},
                          ExpenseTree{{{Category{"Christmas"}, 800_cents},
                                       {Category{"Birthdays"}, 900_cents},
                                       {Category{"Anniversary"}, 1000_cents}}}},
                         {Category{"Entertainment"}, 1100_cents},
                         {Category{"Car Loans"},
                          ExpenseTree{{{Category{"Honda"}, 1200_cents},
                                       {Category{"Ford"}, 1300_cents}}}}}},
            RecursiveCategory{
                Category{"Food"},
                Subcategory{RecursiveCategory{
                    Category{"Dining Out"},
                    Subcategory{RecursiveCategory{Category{"Chipotle"}}}}}}));
}

void totalHavingMultipleExpenseTrees(testcpplite::TestResult &result) {
  assertEqual(
      result, 9000_cents,
      total(ExpenseTree{
          {{Category{"Food"},
            ExpenseTree{{{Category{"Dining Out"}, 200_cents},
                         {Category{"Groceries"}, 300_cents}}}},
           {Category{"Phone"}, ExpenseTree{{{Category{"Verizon"}, 400_cents},
                                            {Category{"Light"}, 500_cents}}}},
           {Category{"Health"}, ExpenseTree{{{Category{"Gym"}, 600_cents},
                                             {Category{"Other"}, 700_cents}}}},
           {Category{"Gifts"},
            ExpenseTree{{{Category{"Christmas"}, 800_cents},
                         {Category{"Birthdays"}, 900_cents},
                         {Category{"Anniversary"}, 1000_cents}}}},
           {Category{"Entertainment"}, 1100_cents},
           {Category{"Car Loans"},
            ExpenseTree{{{Category{"Honda"}, 1200_cents},
                         {Category{"Ford"}, 1300_cents}}}}}}));
}

static void assertEqual(testcpplite::TestResult &result,
                        const Categories &expected, const Categories &actual) {
  assertEqual(result, expected.each.size(), actual.each.size());
  assertTrue(result, std::equal(expected.each.begin(), expected.each.end(),
                                actual.each.begin()));
}

void categoriesFromNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, Categories{}, categories(Expenses{}));
}

void categoriesFromOneExpense(testcpplite::TestResult &result) {
  assertEqual(result, Categories{{Category{"groceries"}}},
              categories(Expenses{{Expense{0_cents, Category{"groceries"}}}}));
}

void categoriesFromTwoExpensesOfSameCategory(testcpplite::TestResult &result) {
  assertEqual(result, Categories{{Category{"groceries"}}},
              categories(Expenses{{Expense{1_cents, Category{"groceries"}},
                                   Expense{2_cents, Category{"groceries"}}}}));
}
} // namespace sbash64::budget::calculate
