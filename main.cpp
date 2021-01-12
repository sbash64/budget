#include <fstream>
#include <iostream>
#include <sbash64/budget/account.hpp>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/budget/print.hpp>
#include <sbash64/budget/stream.hpp>

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
  StreamView printer{std::cout};
  FileStreamFactory streamFactory;
  WritesSessionToStream outputStream{streamFactory};
  ReadsSessionFromStream inputStream{streamFactory};
  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    if (line == "exit")
      break;
    if (line == "save") {
      bank.save(outputStream);
    } else if (line == "load") {
      bank.load(inputStream);
    } else
      sbash64::budget::command(controller, bank, printer, line);
  }
}
} // namespace sbash64::budget

int main() { sbash64::budget::repl(); }