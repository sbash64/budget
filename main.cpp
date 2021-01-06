#include <fstream>
#include <iostream>
#include <sbash64/budget/account.hpp>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/budget/file.hpp>
#include <sbash64/budget/print.hpp>

namespace sbash64::budget {
static void repl() {
  Controller controller;
  InMemoryAccount::Factory accountFactory;
  Bank bank{accountFactory};
  StreamPrinter printer{std::cout};
  std::ofstream fileStream;
  fileStream.open("budget.txt");
  File file{fileStream};
  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    if (line == "exit")
      break;
    sbash64::budget::command(controller, bank, printer, file, line);
  }
  fileStream.close();
}
} // namespace sbash64::budget

int main() { sbash64::budget::repl(); }