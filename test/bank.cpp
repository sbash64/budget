#include "bank.hpp"
#include <sbash64/budget/bank.hpp>
#include <sbash64/testcpplite/testcpplite.hpp>

namespace sbash64::budget::bank {
namespace {
class AccountFactoryStub : public AccountFactory {
public:
  auto name() -> std::string { return name_; }

  auto make(std::string_view s) -> std::shared_ptr<Account> override {
    name_ = s;
    return {};
  }

private:
  std::string name_;
};
} // namespace

void createsMasterAccountOnConstruction(testcpplite::TestResult &result) {
  AccountFactoryStub factory;
  Bank bank{factory};
  assertEqual(result, "master", factory.name());
}
} // namespace sbash64::budget::bank
