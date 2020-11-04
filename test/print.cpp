#include "print.hpp"
#include "usd.hpp"
#include <sbash64/budget/print.hpp>
#include <sstream>

namespace sbash64::budget::print {
static void
assertPrettyWithBoundedNewlinesYields(testcpplite::TestResult &result,
                                      Income income, const Expenses &expenses,
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
  assertPrettyWithBoundedNewlinesYields(result, Income{}, Expenses{}, R"(
Income: $0.00
Expenses: $0.00
Difference: $0.00
)");
}

void prettyBudgetHavingNoExpenses(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(result, Income{100_cents}, Expenses{},
                                        R"(
Income: $1.00
Expenses: $0.00
Difference: $1.00
)");
}

void prettyBudgetHavingOneExpense(testcpplite::TestResult &result) {
  assertPrettyWithBoundedNewlinesYields(
      result, Income{100_cents},
      Expenses{{Expense{25_cents, Category{"Groceries"}}}},
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
      Expenses{{Expense{1234_cents, Category{"Groceries"}},
                Expense{5678_cents, Category{"Groceries"}},
                Expense{342_cents, Category{"Entertainment"}},
                Expense{598_cents, Category{"Groceries"}},
                Expense{2999_cents, Category{"Rent"}},
                Expense{1266_cents, Category{"Giving"}},
                Expense{3538_cents, Category{"Gas"}},
                Expense{3242_cents, Category{"Rent"}},
                Expense{5893_cents, Category{"Gas"}}}},
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
