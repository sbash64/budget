#ifndef SBASH64_BUDGET_SERIALIZATION_HPP_
#define SBASH64_BUDGET_SERIALIZATION_HPP_

#include "budget.hpp"
#include <istream>
#include <memory>
#include <ostream>

namespace sbash64::budget {
class IoStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(IoStreamFactory);
  virtual auto makeInput() -> std::shared_ptr<std::istream> = 0;
  virtual auto makeOutput() -> std::shared_ptr<std::ostream> = 0;
};

class WritesSessionToStream : public SessionSerialization {
public:
  explicit WritesSessionToStream(IoStreamFactory &);
  void save(Account &primary,
            const std::vector<Account *> &secondaries) override;
  void saveAccount(std::string_view name,
                   const std::vector<Transaction> &credits,
                   const std::vector<Transaction> &debits) override;

private:
  IoStreamFactory &ioStreamFactory;
  std::ostream *stream{};
};

class ReadsSessionFromStream : public SessionDeserialization {
public:
  explicit ReadsSessionFromStream(IoStreamFactory &);
  void loadAccount(std::vector<Transaction> &credits,
                   std::vector<Transaction> &debits) override;
  void load(Account::Factory &factory, std::shared_ptr<Account> &primary,
            std::map<std::string, std::shared_ptr<Account>, std::less<>>
                &secondaries) override;

private:
  IoStreamFactory &ioStreamFactory;
  std::istream *stream{};
};
} // namespace sbash64::budget

#endif
