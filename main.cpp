#include "./test/persistent-memory-stub.hpp"
#include <iostream>
#include <sbash64/budget/account.hpp>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/budget/print.hpp>

namespace sbash64::budget {
static void repl() {
  Controller controller;
  InMemoryAccount::Factory accountFactory;
  Bank bank{accountFactory};
  StreamPrinter printer{std::cout};
  PersistentMemoryStub persistentMemory;
  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    sbash64::budget::command(controller, bank, printer, persistentMemory, line);
  }
}
} // namespace sbash64::budget

int main() { sbash64::budget::repl(); }