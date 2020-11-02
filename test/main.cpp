#include "parse.hpp"
#include <iostream>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64 {
namespace budget {
int main() {
  testcpplite::test({{parse::zero, "parse zero"},
                     {parse::one, "parse one"},
                     {parse::twoDecimalPlaces, "parse two decimal places"}},
                    std::cout);
}
} // namespace budget
} // namespace sbash64

int main() { return sbash64::budget::main(); }
