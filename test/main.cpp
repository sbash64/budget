#include "calculate.hpp"
#include "parse.hpp"
#include "print.hpp"
#include <iostream>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64 {
namespace budget {
int main() {
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
       {calculate::categoryTotalHavingNoExpenses,
        "calculate category total having no expenses"},
       {calculate::categoryTotalHavingOneUnrelatedExpense,
        "calculate category total having one unrelated expense"},
       {calculate::categoryTotalHavingOneExpense,
        "calculate category total having one expense"},
       {print::prettyBudgetHavingNoIncomeNorExpenses,
        "print pretty budget having no income nor expenses"},
       {print::formatZeroDollars, "format zero dollars"},
       {print::formatOneDollar, "format one dollar"},
       {print::formatOneCent, "format one cent"}},
      std::cout);
}
} // namespace budget
} // namespace sbash64

int main() { return sbash64::budget::main(); }
