#include "calculate.hpp"
#include "usd.hpp"
#include <algorithm>
#include <sbash64/budget/calculate.hpp>

namespace sbash64::budget::calculate {
void surplusHavingNoIncomeNorExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, surplus(Income{0_cents}, ExpenseTree{}));
}

void surplusHavingNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 600_cents, surplus(Income{600_cents}, ExpenseTree{}));
}

void surplusHavingOneExpense(testcpplite::TestResult &result) {
  assertEqual(
      result, 500_cents,
      surplus(Income{600_cents}, ExpenseTree{{
                                     {Category{"Entertainment"}, 100_cents},
                                 }}));
}

void surplusHavingTwoExpenses(testcpplite::TestResult &result) {
  assertEqual(
      result, 300_cents,
      surplus(Income{600_cents}, ExpenseTree{{
                                     {Category{"Entertainment"}, 100_cents},
                                     {Category{"Groceries"}, 200_cents},
                                 }}));
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

void surplusAfterExpenseChange(testcpplite::TestResult &result) {
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

void expenseCategoryTotalHavingNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, total(ExpenseTree{}, Category{"Entertainment"}));
}

void expenseCategoryTotalHavingOneUnrelatedExpense(
    testcpplite::TestResult &result) {
  assertEqual(result, 0_cents,
              total(ExpenseTree{{
                        {Category{"Entertainment"}, 1100_cents},
                    }},
                    Category{"Groceries"}));
}

void expenseCategoryTotalHavingOneExpense(testcpplite::TestResult &result) {
  assertEqual(result, 1100_cents,
              total(ExpenseTree{{
                        {Category{"Entertainment"}, 1100_cents},
                    }},
                    Category{"Entertainment"}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories(
    testcpplite::TestResult &result) {
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

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories2(
    testcpplite::TestResult &result) {
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

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories3(
    testcpplite::TestResult &result) {
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
            RecursiveExpenseCategory{
                Category{"Phone"}, ExpenseSubcategory{RecursiveExpenseCategory{
                                       Category{"Verizon"}}}}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories4(
    testcpplite::TestResult &result) {
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
            RecursiveExpenseCategory{
                Category{"Health"}, ExpenseSubcategory{RecursiveExpenseCategory{
                                        Category{"Vitamins"}}}}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories5(
    testcpplite::TestResult &result) {
  assertEqual(
      result, 0_cents,
      total(
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
          RecursiveExpenseCategory{Category{"Vacation"},
                                   ExpenseSubcategory{RecursiveExpenseCategory{
                                       Category{"Car Rental"}}}}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories6(
    testcpplite::TestResult &result) {
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
            RecursiveExpenseCategory{
                Category{"Food"},
                ExpenseSubcategory{RecursiveExpenseCategory{
                    Category{"Dining Out"},
                    ExpenseSubcategory{
                        RecursiveExpenseCategory{Category{"Chipotle"}}}}}}));
}

void expenseTotalHavingMultipleExpenseCategoriesAndSubcategories(
    testcpplite::TestResult &result) {
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
} // namespace sbash64::budget::calculate
