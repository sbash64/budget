#ifndef SBASH64_BUDGET_STREAM_HPP_
#define SBASH64_BUDGET_STREAM_HPP_

#include "budget.hpp"
#include <iostream>
#include <memory>

namespace sbash64::budget {
class IoStreamFactory {
public:
  SBASH64_BUDGET_INTERFACE_SPECIAL_MEMBER_FUNCTIONS(IoStreamFactory);
  virtual auto make() -> std::shared_ptr<std::iostream> = 0;
};

class OutputStream : public OutputPersistentMemory {
public:
  explicit OutputStream(IoStreamFactory &);
  void save(Account &primary,
            const std::vector<Account *> &secondaries) override;
  void saveAccount(std::string_view name,
                   const std::vector<Transaction> &credits,
                   const std::vector<Transaction> &debits) override;

private:
  IoStreamFactory &ioStreamFactory;
  std::ostream *stream{};
};

class InputStream : public InputPersistentMemory {
public:
  explicit InputStream(IoStreamFactory &);
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
