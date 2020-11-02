#include "parse.hpp"
#include <iostream>
#include <sbash64/testcpplite/testcpplite.hpp>

int main() {
  sbash64::testcpplite::test({{sbash64::budget::parse::zero, "parse zero"}},
                             std::cout);
}
