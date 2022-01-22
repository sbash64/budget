#ifndef SBASH64_BUDGET_TEST_ACCOUNT_OBSERVER_HPP_
#define SBASH64_BUDGET_TEST_ACCOUNT_OBSERVER_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::presentation {
void formatsTransactionAmount(testcpplite::TestResult &);
void formatsTransactionDate(testcpplite::TestResult &);
void formatsAccountBalance(testcpplite::TestResult &);
void formatsAccountAllocation(testcpplite::TestResult &);
void passesDescriptionOfNewTransaction(testcpplite::TestResult &);
void ordersTransactionsByMostRecentDate(testcpplite::TestResult &);
void ordersSameDateTransactionsByDescription(testcpplite::TestResult &);
void putsCheckmarkNextToVerifiedTransaction(testcpplite::TestResult &);
void deletesRemovedTransactionRow(testcpplite::TestResult &);
} // namespace sbash64::budget::presentation

#endif
