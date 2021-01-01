#include <iostream>
#include <sbash64/budget/evaluate.hpp>

namespace sbash64::budget {
namespace {
class TBDRecord : public ExpenseRecord {
public:
  void enter(const LabeledExpense &) {}
  void print(std::ostream &) {}
};
} // namespace

static void repl() {
  TBDRecord record;
  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    sbash64::budget::evaluate::command(record, line, std::cout);
    std::cout << '\n';
  }
}
} // namespace sbash64::budget

int main() { sbash64::budget::repl(); }