#include <forward_list>
#include <iostream>
#include <sbash64/budget/evaluate.hpp>
#include <sbash64/budget/print.hpp>
#include <string>
#include <vector>

namespace sbash64::budget {
namespace {
struct TBDExpense {
  std::vector<Category> categories;
  std::string label;
  USD usd;
};
} // namespace

static void initialize(TBDExpense &converted, const RecursiveExpense &expense) {
  converted.categories.push_back(expense.category);
  if (std::holds_alternative<Subexpense>(expense.subexpenseOrUsd)) {
    initialize(converted, std::get<Subexpense>(expense.subexpenseOrUsd));
  } else {
    converted.usd = std::get<USD>(expense.subexpenseOrUsd);
  }
}

static auto tbdExpense(const LabeledExpense &expense) -> TBDExpense {
  TBDExpense converted;
  converted.label = expense.label;
  initialize(converted, expense.expense);
  return converted;
}

static void initialize(RecursiveExpense &expense, const TBDExpense &converted,
                       int index,
                       std::forward_list<RecursiveExpense> &inMemory) {
  if (index < converted.categories.size()) {
    inMemory.push_front({});
    auto &subexpense{inMemory.front()};
    subexpense.category = converted.categories.at(index);
    ++index;
    initialize(subexpense, converted, index, inMemory);
    expense.subexpenseOrUsd.emplace<Subexpense>(subexpense);
  } else {
    expense.subexpenseOrUsd = converted.usd;
  }
}

static auto labeledExpense(const TBDExpense &converted,
                           std::forward_list<RecursiveExpense> &inMemory)
    -> LabeledExpense {
  LabeledExpense expense;
  expense.label = converted.label;
  int index{1};
  expense.expense.category = converted.categories.front();
  initialize(expense.expense, converted, index, inMemory);
  return expense;
}

static auto labeledExpenses(const std::vector<TBDExpense> &converted,
                            std::forward_list<RecursiveExpense> &inMemory)
    -> std::vector<LabeledExpense> {
  std::vector<LabeledExpense> expenses;
  for (auto c : converted)
    expenses.push_back(labeledExpense(c, inMemory));
  return expenses;
}

namespace {
class TBDRecord : public ExpenseRecord {
public:
  void enter(const LabeledExpense &expense) {
    expenses.push_back(tbdExpense(expense));
  }

  void print(std::ostream &stream) {
    std::forward_list<RecursiveExpense> inMemory;
    print::pretty(stream, labeledExpenses(expenses, inMemory));
  }

private:
  std::vector<TBDExpense> expenses;
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