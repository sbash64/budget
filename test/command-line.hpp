#ifndef SBASH64_BUDGET_TEST_COMMAND_LINE_HPP_
#define SBASH64_BUDGET_TEST_COMMAND_LINE_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::command_line {
void print(testcpplite::TestResult &);
void debit(testcpplite::TestResult &);
void credit(testcpplite::TestResult &);
void transferTo(testcpplite::TestResult &);
void save(testcpplite::TestResult &);
void load(testcpplite::TestResult &);
void debitPromptsForDate(testcpplite::TestResult &);
void debitPromptsForDesriptionAfterDateEntered(testcpplite::TestResult &);
void debitShowsTransaction(testcpplite::TestResult &);
} // namespace sbash64::budget::command_line

#endif