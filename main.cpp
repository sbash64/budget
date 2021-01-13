#include <fstream>
#include <iostream>
#include <sbash64/budget/account.hpp>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/command-line.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sbash64/budget/view.hpp>

namespace sbash64::budget {
class FileStreamFactory : public IoStreamFactory {
public:
  auto makeInput() -> std::shared_ptr<std::istream> override {
    return std::make_shared<std::ifstream>("budget.txt");
  }

  auto makeOutput() -> std::shared_ptr<std::ostream> override {
    return std::make_shared<std::ofstream>("budget.txt");
  }
};

static void repl() {
  Controller controller;
  InMemoryAccount::Factory accountFactory;
  Bank bank{accountFactory};
  StreamView view{std::cout};
  FileStreamFactory streamFactory;
  WritesSessionToStream serialization{streamFactory};
  ReadsSessionFromStream deserialization{streamFactory};
  for (;;) {
    std::string line;
    getline(std::cin, line);
    if (line == "exit")
      break;
    command(controller, bank, view, serialization, deserialization, line);
  }
}
} // namespace sbash64::budget

int main() { sbash64::budget::repl(); }