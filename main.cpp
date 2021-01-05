#include <iostream>
#include <sbash64/budget/account.hpp>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/budget/print.hpp>

namespace sbash64::budget {
static void repl() {
  evaluate::Controller controller;
  InMemoryAccount::Factory accountFactory;
  Bank bank{accountFactory};
  print::StreamPrinter printer{std::cout};
  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    sbash64::budget::evaluate::command(controller, bank, printer, line);
  }
}
} // namespace sbash64::budget

int main() { sbash64::budget::repl(); }