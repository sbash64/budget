#ifndef SBASH64_BUDGET_TEST_EVALUATE_HPP_
#define SBASH64_BUDGET_TEST_EVALUATE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::evaluate {
void print(testcpplite::TestResult &);
void debit(testcpplite::TestResult &);
void credit(testcpplite::TestResult &);
void transferTo(testcpplite::TestResult &);
void save(testcpplite::TestResult &);
} // namespace sbash64::budget::evaluate

#endif
