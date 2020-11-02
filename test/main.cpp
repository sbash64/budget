#include "parse.hpp"
#include <iostream>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64 {
namespace budget {
int main() {
  testcpplite::test({{parse::zero, "parse zero"},
                     {parse::one, "parse one"},
                     {parse::twoDecimalPlaces, "parse two decimal places"},
                     {parse::oneDecimalPlace, "parse one decimal place"},
                     {parse::oneOneHundredth, "parse one one-hundredth"},
                     {parse::oneTenth, "parse ten"},
                     {parse::withoutLeadingZero, "parse without leading zero"},
                     {parse::threeDecimalPlaces, "parse three decimal places"}},
                    std::cout);
}
} // namespace budget
} // namespace sbash64

int main() { return sbash64::budget::main(); }
