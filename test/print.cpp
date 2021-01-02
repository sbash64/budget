#include "print.hpp"
#include "usd.hpp"
#include <sbash64/budget/print.hpp>
#include <sstream>

namespace sbash64::budget::print {
static void assertPrettyWithBoundedNewlinesYields(
    testcpplite::TestResult &result, Income income, const ExpenseTree &expenses,
    std::string_view expected) {
  std::stringstream stream;
  pretty(stream, income, expenses);
  assertEqual(result, expected.data(), '\n' + stream.str() + '\n');
}

static void assertPrettyWithBoundedNewlinesYields(
    testcpplite::TestResult &result,
    const std::vector<LabeledExpense> &expenses, std::string_view expected) {
  std::stringstream stream;
  pretty(stream, expenses);
  assertEqual(result, expected.data(), '\n' + stream.str() + '\n');
}

static void assertPrettyWithBoundedNewlinesYields(
    testcpplite::TestResult &result,
    const std::vector<PrintableTransaction> &transactions,
    std::string_view expected) {
  std::stringstream stream;
  pretty(stream, transactions);
  assertEqual(result, expected.data(), '\n' + stream.str() + '\n');
}

static void assertFormatYields(testcpplite::TestResult &result, USD usd,
                               std::string_view expected) {
  assertEqual(result, expected.data(), format(usd));
}

void prettyBudgetHavingNoIncomeNorExpenses(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(result, Income{}, ExpenseTree{}, R"(
Income: $0.00
Expenses: $0.00
Difference: $0.00
)");
}

void prettyBudgetHavingNoExpenses(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(result, Income{100_cents},
                                        ExpenseTree{},
                                        R"(
Income: $1.00
Expenses: $0.00
Difference: $1.00
)");
}

void prettyBudgetHavingOneExpense(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(result, Income{100_cents},
                                        ExpenseTree{{
                                            {Category{"Groceries"}, 25_cents},
                                        }},
                                        R"(
Income: $1.00
Expenses: $0.25
    Groceries: $0.25
Difference: $0.75
)");
}

void prettyBudgetHavingMultipleExpenses(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(
      result, Income{25000_cents},
      ExpenseTree{{
          {Category{"Entertainment"}, 342_cents},
          {Category{"Giving"}, 1266_cents},
          {Category{"Rent"}, 6241_cents},
          {Category{"Groceries"}, 7510_cents},
          {Category{"Gas"}, 9431_cents},
      }},
      R"(
Income: $250.00
Expenses: $247.90
    Entertainment: $3.42
    Gas: $94.31
    Giving: $12.66
    Groceries: $75.10
    Rent: $62.41
Difference: $2.10
)");
}

void prettyBudgetHavingMultipleExpenseTrees(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(
      result, Income{10000_cents},
      ExpenseTree{
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
                         {Category{"Ford"}, 1300_cents}}}}}},
      R"(
Income: $100.00
Expenses: $90.00
    Car Loans: $25.00
        Ford: $13.00
        Honda: $12.00
    Entertainment: $11.00
    Food: $5.00
        Dining Out: $2.00
        Groceries: $3.00
    Gifts: $27.00
        Anniversary: $10.00
        Birthdays: $9.00
        Christmas: $8.00
    Health: $13.00
        Gym: $6.00
        Other: $7.00
    Phone: $9.00
        Light: $5.00
        Verizon: $4.00
Difference: $10.00
)");
}

void formatZeroDollars(testcpplite::TestResult &result) {
  assertFormatYields(result, 0_cents, "$0.00");
}

void formatOneDollar(testcpplite::TestResult &result) {
  assertFormatYields(result, 100_cents, "$1.00");
}

void formatOneCent(testcpplite::TestResult &result) {
  assertFormatYields(result, 1_cents, "$0.01");
}

void formatTenCents(testcpplite::TestResult &result) {
  assertFormatYields(result, 10_cents, "$0.10");
}

void aFewExpenses(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(
      result,
      {LabeledExpense{RecursiveExpense{Category{"Gifts"},
                                       Subexpense{RecursiveExpense{
                                           Category{"Birthdays"}, 2500_cents}}},
                      "Sam's 24th"},
       LabeledExpense{RecursiveExpense{Category{"Gifts"},
                                       Subexpense{RecursiveExpense{
                                           Category{"Birthdays"}, 1500_cents}}},
                      "Brinley's 3rd"},
       LabeledExpense{
           RecursiveExpense{Category{"Food"},
                            Subexpense{RecursiveExpense{Category{"Dining Out"},
                                                        1300_cents}}},
           "Chipotle 10/30/20"},
       LabeledExpense{RecursiveExpense{Category{"Phone"},
                                       Subexpense{RecursiveExpense{
                                           Category{"Verizon"}, 6700_cents}}},
                      "Seren's Galaxy"},
       LabeledExpense{RecursiveExpense{Category{"Food"},
                                       Subexpense{RecursiveExpense{
                                           Category{"Groceries"}, 3700_cents}}},
                      "Hyvee 10/15/20"},
       LabeledExpense{RecursiveExpense{Category{"Food"},
                                       Subexpense{RecursiveExpense{
                                           Category{"Dining Out"}, 850_cents}}},
                      "Raising Cane's 10/17/20"}},
      R"(
Gifts::Birthdays: $25.00 - Sam's 24th
Gifts::Birthdays: $15.00 - Brinley's 3rd
Food::Dining Out: $13.00 - Chipotle 10/30/20
Phone::Verizon: $67.00 - Seren's Galaxy
Food::Groceries: $37.00 - Hyvee 10/15/20
Food::Dining Out: $8.50 - Raising Cane's 10/17/20
)");
}

void transactions(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(
      result,
      {printableDebit(Transaction{2500_cents, "Sam's 24th",
                                  Date{2020, Month::December, 27}}),
       printableDebit(Transaction{2734_cents, "Brinley's 3rd",
                                  Date{2021, Month::January, 14}}),
       printableDebit(Transaction{2410_cents, "Hannah's 30th",
                                  Date{2021, Month::March, 18}})},
      R"(
Debit ($)   Credit ($)   Date (mm/dd/yyyy)   Description
25.00                    12/27/2020          Sam's 24th
27.34                    01/14/2021          Brinley's 3rd
24.10                    03/18/2021          Hannah's 30th
)");
}
} // namespace sbash64::budget::print
