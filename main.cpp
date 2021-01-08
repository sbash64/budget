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
  StreamView printer{std::cout};
  std::ofstream outputFileStream;
  std::ifstream inputFileStream{"budget.txt"};
  OutputStream outputStream{outputFileStream};
  InputStream inputStream{inputFileStream};
  if (inputFileStream.is_open())
    bank.load(inputStream);
  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    if (line == "exit")
      break;
    if (line == "save") {
      outputFileStream.open("budget.txt");
      bank.save(outputStream);
      outputFileStream.close();
    } else
      sbash64::budget::command(controller, bank, printer, line);
  }
}
} // namespace sbash64::budget

int main() { sbash64::budget::repl(); }