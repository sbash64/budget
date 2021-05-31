#ifndef SBASH64_BUDGET_TEST_ENTERING_TRANSACTION_CONTROLLER_HPP_
#define SBASH64_BUDGET_TEST_ENTERING_TRANSACTION_CONTROLLER_HPP_

#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::entering_transaction::controller {
void shouldTranslateControlDataToDebit(sbash64::testcpplite::TestResult &);
void shouldTranslateControlDataToCredit(sbash64::testcpplite::TestResult &);
} // namespace sbash64::budget::entering_transaction::controller

#endif
