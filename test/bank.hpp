#ifndef SBASH64_BUDGET_TEST_BANK_HPP_
#define SBASH64_BUDGET_TEST_BANK_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::bank {
void createsMasterAccountOnConstruction(testcpplite::TestResult &);
void creditsMasterAccountWhenCredited(testcpplite::TestResult &);
void debitsNonexistantAccount(testcpplite::TestResult &);
void debitsExistingAccount(testcpplite::TestResult &);
void transferDebitsMasterAndCreditsOther(testcpplite::TestResult &);
void printPrintsAccounts(testcpplite::TestResult &);
} // namespace sbash64::budget::bank

#endif
