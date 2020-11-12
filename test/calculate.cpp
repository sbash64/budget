#include "calculate.hpp"
#include "usd.hpp"
#include <algorithm>
#include <sbash64/budget/calculate.hpp>

namespace sbash64::budget::calculate {
void surplusHavingNoIncomeNorExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents, surplus(Income{0_cents}, Expenses{}));
}

void surplusHavingNoExpenses(testcpplite::TestResult &result) {
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
          ExpenseTree{
              {{ExpenseCategory{"Food"},
                ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                             {ExpenseCategory{"Groceries"}, 300_cents}}}},
               {ExpenseCategory{"Phone"},
                ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                             {ExpenseCategory{"Light"}, 500_cents}}}},
               {ExpenseCategory{"Health"},
                ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                             {ExpenseCategory{"Other"}, 700_cents}}}},
               {ExpenseCategory{"Gifts"},
                ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                             {ExpenseCategory{"Birthdays"}, 900_cents},
                             {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
               {ExpenseCategory{"Entertainment"}, 1100_cents},
               {ExpenseCategory{"Car Loans"},
                ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                             {ExpenseCategory{"Ford"}, 1300_cents}}}}}}));
}

void surplusAfterExpenseChange(testcpplite::TestResult &result) {
  assertEqual(
      result, 800_cents,
      surplus(
          Income{10000_cents},
          ExpenseTree{
              {{ExpenseCategory{"Food"},
                ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                             {ExpenseCategory{"Groceries"}, 300_cents}}}},
               {ExpenseCategory{"Phone"},
                ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                             {ExpenseCategory{"Light"}, 500_cents}}}},
               {ExpenseCategory{"Health"},
                ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                             {ExpenseCategory{"Other"}, 700_cents}}}},
               {ExpenseCategory{"Gifts"},
                ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                             {ExpenseCategory{"Birthdays"}, 900_cents},
                             {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
               {ExpenseCategory{"Entertainment"}, 1100_cents},
               {ExpenseCategory{"Car Loans"},
                ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                             {ExpenseCategory{"Ford"}, 1300_cents}}}}}},
          RecursiveExpense{ExpenseCategory{"Gifts"},
                           Subexpense{RecursiveExpense{
                               ExpenseCategory{"Birthdays"}, 1100_cents}}}));
}

void expenseCategoryTotalHavingNoExpenses(testcpplite::TestResult &result) {
  assertEqual(result, 0_cents,
              total(ExpenseTree{}, ExpenseCategory{"Entertainment"}));
}

void expenseCategoryTotalHavingOneUnrelatedExpense(
    testcpplite::TestResult &result) {
  assertEqual(result, 0_cents,
              total(ExpenseTree{{
                        {ExpenseCategory{"Entertainment"}, 1100_cents},
                    }},
                    ExpenseCategory{"Groceries"}));
}

void expenseCategoryTotalHavingOneExpense(testcpplite::TestResult &result) {
  assertEqual(result, 1100_cents,
              total(ExpenseTree{{
                        {ExpenseCategory{"Entertainment"}, 1100_cents},
                    }},
                    ExpenseCategory{"Entertainment"}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories(
    testcpplite::TestResult &result) {
  assertEqual(
      result, 2700_cents,
      total(
          ExpenseTree{
              {{ExpenseCategory{"Food"},
                ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                             {ExpenseCategory{"Groceries"}, 300_cents}}}},
               {ExpenseCategory{"Phone"},
                ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                             {ExpenseCategory{"Light"}, 500_cents}}}},
               {ExpenseCategory{"Health"},
                ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                             {ExpenseCategory{"Other"}, 700_cents}}}},
               {ExpenseCategory{"Gifts"},
                ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                             {ExpenseCategory{"Birthdays"}, 900_cents},
                             {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
               {ExpenseCategory{"Entertainment"}, 1100_cents},
               {ExpenseCategory{"Car Loans"},
                ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                             {ExpenseCategory{"Ford"}, 1300_cents}}}}}},
          ExpenseCategory{"Gifts"}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories2(
    testcpplite::TestResult &result) {
  assertEqual(
      result, 1100_cents,
      total(
          ExpenseTree{
              {{ExpenseCategory{"Food"},
                ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                             {ExpenseCategory{"Groceries"}, 300_cents}}}},
               {ExpenseCategory{"Phone"},
                ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                             {ExpenseCategory{"Light"}, 500_cents}}}},
               {ExpenseCategory{"Health"},
                ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                             {ExpenseCategory{"Other"}, 700_cents}}}},
               {ExpenseCategory{"Gifts"},
                ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                             {ExpenseCategory{"Birthdays"}, 900_cents},
                             {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
               {ExpenseCategory{"Entertainment"}, 1100_cents},
               {ExpenseCategory{"Car Loans"},
                ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                             {ExpenseCategory{"Ford"}, 1300_cents}}}}}},
          ExpenseCategory{"Entertainment"}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories3(
    testcpplite::TestResult &result) {
  assertEqual(
      result, 400_cents,
      total(
          ExpenseTree{
              {{ExpenseCategory{"Food"},
                ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                             {ExpenseCategory{"Groceries"}, 300_cents}}}},
               {ExpenseCategory{"Phone"},
                ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                             {ExpenseCategory{"Light"}, 500_cents}}}},
               {ExpenseCategory{"Health"},
                ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                             {ExpenseCategory{"Other"}, 700_cents}}}},
               {ExpenseCategory{"Gifts"},
                ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                             {ExpenseCategory{"Birthdays"}, 900_cents},
                             {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
               {ExpenseCategory{"Entertainment"}, 1100_cents},
               {ExpenseCategory{"Car Loans"},
                ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                             {ExpenseCategory{"Ford"}, 1300_cents}}}}}},
          RecursiveExpenseCategory{ExpenseCategory{"Phone"},
                                   ExpenseSubcategory{RecursiveExpenseCategory{
                                       ExpenseCategory{"Verizon"}}}}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories4(
    testcpplite::TestResult &result) {
  assertEqual(
      result, 0_cents,
      total(
          ExpenseTree{
              {{ExpenseCategory{"Food"},
                ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                             {ExpenseCategory{"Groceries"}, 300_cents}}}},
               {ExpenseCategory{"Phone"},
                ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                             {ExpenseCategory{"Light"}, 500_cents}}}},
               {ExpenseCategory{"Health"},
                ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                             {ExpenseCategory{"Other"}, 700_cents}}}},
               {ExpenseCategory{"Gifts"},
                ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                             {ExpenseCategory{"Birthdays"}, 900_cents},
                             {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
               {ExpenseCategory{"Entertainment"}, 1100_cents},
               {ExpenseCategory{"Car Loans"},
                ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                             {ExpenseCategory{"Ford"}, 1300_cents}}}}}},
          RecursiveExpenseCategory{ExpenseCategory{"Health"},
                                   ExpenseSubcategory{RecursiveExpenseCategory{
                                       ExpenseCategory{"Vitamins"}}}}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories5(
    testcpplite::TestResult &result) {
  assertEqual(
      result, 0_cents,
      total(
          ExpenseTree{
              {{ExpenseCategory{"Food"},
                ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                             {ExpenseCategory{"Groceries"}, 300_cents}}}},
               {ExpenseCategory{"Phone"},
                ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                             {ExpenseCategory{"Light"}, 500_cents}}}},
               {ExpenseCategory{"Health"},
                ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                             {ExpenseCategory{"Other"}, 700_cents}}}},
               {ExpenseCategory{"Gifts"},
                ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                             {ExpenseCategory{"Birthdays"}, 900_cents},
                             {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
               {ExpenseCategory{"Entertainment"}, 1100_cents},
               {ExpenseCategory{"Car Loans"},
                ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                             {ExpenseCategory{"Ford"}, 1300_cents}}}}}},
          RecursiveExpenseCategory{ExpenseCategory{"Vacation"},
                                   ExpenseSubcategory{RecursiveExpenseCategory{
                                       ExpenseCategory{"Car Rental"}}}}));
}

void expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories6(
    testcpplite::TestResult &result) {
  assertEqual(
      result, 0_cents,
      total(
          ExpenseTree{
              {{ExpenseCategory{"Food"},
                ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                             {ExpenseCategory{"Groceries"}, 300_cents}}}},
               {ExpenseCategory{"Phone"},
                ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                             {ExpenseCategory{"Light"}, 500_cents}}}},
               {ExpenseCategory{"Health"},
                ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                             {ExpenseCategory{"Other"}, 700_cents}}}},
               {ExpenseCategory{"Gifts"},
                ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                             {ExpenseCategory{"Birthdays"}, 900_cents},
                             {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
               {ExpenseCategory{"Entertainment"}, 1100_cents},
               {ExpenseCategory{"Car Loans"},
                ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                             {ExpenseCategory{"Ford"}, 1300_cents}}}}}},
          RecursiveExpenseCategory{
              ExpenseCategory{"Food"},
              ExpenseSubcategory{RecursiveExpenseCategory{
                  ExpenseCategory{"Dining Out"},
                  ExpenseSubcategory{RecursiveExpenseCategory{
                      ExpenseCategory{"Chipotle"}}}}}}));
}

void expenseTotalHavingMultipleExpenseCategoriesAndSubcategories(
    testcpplite::TestResult &result) {
  assertEqual(
      result, 9000_cents,
      total(ExpenseTree{
          {{ExpenseCategory{"Food"},
            ExpenseTree{{{ExpenseCategory{"Dining Out"}, 200_cents},
                         {ExpenseCategory{"Groceries"}, 300_cents}}}},
           {ExpenseCategory{"Phone"},
            ExpenseTree{{{ExpenseCategory{"Verizon"}, 400_cents},
                         {ExpenseCategory{"Light"}, 500_cents}}}},
           {ExpenseCategory{"Health"},
            ExpenseTree{{{ExpenseCategory{"Gym"}, 600_cents},
                         {ExpenseCategory{"Other"}, 700_cents}}}},
           {ExpenseCategory{"Gifts"},
            ExpenseTree{{{ExpenseCategory{"Christmas"}, 800_cents},
                         {ExpenseCategory{"Birthdays"}, 900_cents},
                         {ExpenseCategory{"Anniversary"}, 1000_cents}}}},
           {ExpenseCategory{"Entertainment"}, 1100_cents},
           {ExpenseCategory{"Car Loans"},
            ExpenseTree{{{ExpenseCategory{"Honda"}, 1200_cents},
                         {ExpenseCategory{"Ford"}, 1300_cents}}}}}}));
}
} // namespace sbash64::budget::calculate
