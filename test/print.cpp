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
  assertPrettyWithBoundedNewlinesYields(
      result, Income{100_cents},
      ExpenseTree{{
          {ExpenseCategory{"Groceries"}, 25_cents},
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
          {ExpenseCategory{"Entertainment"}, 342_cents},
          {ExpenseCategory{"Giving"}, 1266_cents},
          {ExpenseCategory{"Rent"}, 6241_cents},
          {ExpenseCategory{"Groceries"}, 7510_cents},
          {ExpenseCategory{"Gas"}, 9431_cents},
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
} // namespace sbash64::budget::print
