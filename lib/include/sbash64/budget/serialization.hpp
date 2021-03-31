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

private:
  IoStreamFactory &ioStreamFactory;
};

class WritesAccountToStream : public AccountSerialization {
public:
  explicit WritesAccountToStream(std::ostream &stream);
  void save(std::string_view name, const VerifiableTransactions &credits,
            const VerifiableTransactions &debits) override;

private:
  std::ostream &stream;
};

class ReadsSessionFromStream : public SessionDeserialization {
public:
  explicit ReadsSessionFromStream(IoStreamFactory &);
  void load(Observer &) override;

private:
  IoStreamFactory &ioStreamFactory;
};

class ReadsAccountFromStream : public AccountDeserialization {
public:
  explicit ReadsAccountFromStream(std::istream &);
  void load(Observer &) override;

private:
  std::istream &stream;
};
} // namespace sbash64::budget

#endif
