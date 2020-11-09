#include "calculate.hpp"
#include "parse.hpp"
#include "print.hpp"
#include <iostream>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget {
static auto runAllTests() -> int {
  return testcpplite::test(
      {{parse::zero, "parse zero"},
       {parse::one, "parse one"},
       {parse::twoDecimalPlaces, "parse two decimal places"},
       {parse::oneDecimalPlace, "parse one decimal place"},
       {parse::oneOneHundredth, "parse one one-hundredth"},
       {parse::oneTenth, "parse one tenth"},
       {parse::withoutLeadingZero, "parse without leading zero"},
       {parse::threeDecimalPlaces, "parse three decimal places"},
       {parse::twelveOneThousandths, "parse twelve one thousandths"},
       {calculate::differenceHavingNoIncomeNorExpenses,
        "calculate difference having no income nor expenses"},
       {calculate::differenceHavingIncomeButNoExpenses,
        "calculate difference having income but no expenses"},
       {calculate::differenceHavingOneExpense,
        "calculate difference having one expense"},
       {calculate::differenceHavingTwoExpenses,
        "calculate difference having two expenses"},
       {calculate::differenceHavingMultipleExpenses,
        "calculate difference having multiple expenses"},
       {calculate::differenceAfterUpdate, "calculate difference after update"},
       {calculate::categoryTotalHavingNoExpenses,
        "calculate category total having no expenses"},
       {calculate::categoryTotalHavingOneUnrelatedExpense,
        "calculate category total having one unrelated expense"},
       {calculate::categoryTotalHavingOneExpense,
        "calculate category total having one expense"},
       {calculate::categoryTotalHavingMultipleExpenseTrees,
        "calculate category total having multiple expense trees"},
       {calculate::categoryTotalHavingMultipleExpenseTrees2,
        "calculate category total having multiple expense trees 2"},
       {calculate::categoryTotalHavingMultipleExpenseTrees3,
        "calculate category total having multiple expense trees 3"},
       {calculate::categoryTotalHavingMultipleExpenseTrees4,
        "calculate category total having multiple expense trees 4"},
       {calculate::categoryTotalHavingMultipleExpenseTrees5,
        "calculate category total having multiple expense trees 5"},
       {calculate::categoryTotalHavingMultipleExpenseTrees6,
        "calculate category total having multiple expense trees 6"},
       {calculate::totalHavingMultipleExpenseTrees,
        "calculate category total having multiple expense trees"},
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
