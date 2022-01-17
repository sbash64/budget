#ifndef SBASH64_BUDGET_TEST_ACCOUNT_OBSERVER_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_OBSERVER_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::presentation {
void formatsTransactionAmount(testcpplite::TestResult &);
void formatsDate(testcpplite::TestResult &);
void formatsBalance(testcpplite::TestResult &);
void sendsDescriptionOfNewTransaction(testcpplite::TestResult &);
void ordersTransactionsByMostRecentDate(testcpplite::TestResult &);
} // namespace sbash64::budget::presentation

#endif
