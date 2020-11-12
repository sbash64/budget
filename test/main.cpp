#include "calculate.hpp"
#include "parse.hpp"
#include "print.hpp"
#include <iostream>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
static auto runAllTests() -> int {
  return testcpplite::test(
      {{parse::zeroAsUsd, "parse zero (0) as USD"},
       {parse::oneAsUsd, "parse one (1) as USD"},
       {parse::twoDecimalPlacesAsUsd, "parse two decimal places (X.XX) as USD"},
       {parse::oneDecimalPlaceAsUsd, "parse one decimal place (X.X) as USD"},
       {parse::oneOneHundredthAsUsd, "parse one one-hundredth (0.01) as USD"},
       {parse::oneTenthAsUsd, "parse one-tenth (0.1) as USD"},
       {parse::withoutLeadingZeroAsUsd,
        "parse without leading zero (.X) as USD"},
       {parse::threeDecimalPlacesAsUsd,
        "parse three decimal places (X.XXX) as USD"},
       {parse::twelveOneThousandthsAsUsd,
        "parse twelve one-thousandths (0.012) as USD"},
       {calculate::surplusHavingNoIncomeNorExpenses,
        "calculate surplus having no income nor expenses"},
       {calculate::surplusHavingNoExpenses,
        "calculate surplus having no expenses"},
       {calculate::surplusHavingOneExpense,
        "calculate surplus having one expense"},
       {calculate::surplusHavingTwoExpenses,
        "calculate surplus having two expenses"},
       {calculate::surplusHavingMultipleExpenses,
        "calculate surplus having multiple expenses"},
       {calculate::surplusAfterExpenseChange,
        "calculate surplus after expense change"},
       {calculate::expenseCategoryTotalHavingNoExpenses,
        "calculate expense category total having no expenses"},
       {calculate::expenseCategoryTotalHavingOneUnrelatedExpense,
        "calculate expense category total having one unrelated expense"},
       {calculate::expenseCategoryTotalHavingOneExpense,
        "calculate expense category total having one expense"},
       {calculate::
            expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories,
        "calculate expense category total having multiple expense categories "
        "and subcategories"},
       {calculate::
            expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories2,
        "calculate expense category total having multiple expense categories "
        "and subcategories 2"},
       {calculate::
            expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories3,
        "calculate expense category total having multiple expense categories "
        "and subcategories 3"},
       {calculate::
            expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories4,
        "calculate expense category total having multiple expense categories "
        "and subcategories 4"},
       {calculate::
            expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories5,
        "calculate expense category total having multiple expense categories "
        "and subcategories 5"},
       {calculate::
            expenseCategoryTotalHavingMultipleExpenseCategoriesAndSubcategories6,
        "calculate expense category total having multiple expense categories "
        "and subcategories 6"},
       {calculate::expenseTotalHavingMultipleExpenseCategoriesAndSubcategories,
        "calculate category total having multiple expense categories and "
        "subcategories"},
       {calculate::categoriesFromNoExpenses,
        "calculate categories from no expenses"},
       {calculate::categoriesFromOneExpense,
        "calculate categories from one expense"},
       {calculate::categoriesFromTwoExpensesOfSameCategory,
        "calculate categories from two expenses of same category"},
       {print::prettyBudgetHavingNoIncomeNorExpenses,
        "print pretty budget having no income nor expenses"},
       {print::prettyBudgetHavingNoExpenses,
        "print pretty budget having no expenses"},
       {print::prettyBudgetHavingOneExpense,
        "print pretty budget having one expense"},
       {print::prettyBudgetHavingMultipleExpenses,
        "print pretty budget having multiple expenses"},
       {print::prettyBudgetHavingMultipleExpenseTrees,
        "print pretty budget having multiple expenses trees"},
       {print::formatZeroDollars, "format zero dollars"},
       {print::formatOneDollar, "format one dollar"},
       {print::formatOneCent, "format one cent"},
       {print::formatTenCents, "format ten cents"}},
      std::cout);
}
} // namespace sbash64::budget

int main() { return sbash64::budget::runAllTests(); }
