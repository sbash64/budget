#include <fstream>
#include <gtk/gtk.h>
#include <iostream>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/command-line.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sstream>
#include <string>

namespace sbash64::budget {
class GtkView : public View {
public:
  explicit GtkView(GtkWindow *window) : listBox{gtk_list_box_new()} {
    gtk_window_set_child(window, listBox);
  }

  void show(Account &primary,
            const std::vector<Account *> &secondaries) override {
    primary.show(*this);
    for (auto *secondary : secondaries)
      secondary->show(*this);
  }

  void showAccountSummary(
      std::string_view name, USD balance,
      const std::vector<VerifiableTransactionWithType> &transactions) override {
    auto *expander{gtk_expander_new(std::string{name}.c_str())};
    auto *store{gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING)};
    auto *tree{gtk_tree_view_new_with_model(GTK_TREE_MODEL(store))};
    auto *debitRenderer{gtk_cell_renderer_text_new()};
    auto *debitColumn{gtk_tree_view_column_new_with_attributes(
        "Debit ($)", debitRenderer, "text", 0, NULL)};
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), debitColumn);
    auto *creditRenderer{gtk_cell_renderer_text_new()};
    auto *creditColumn{gtk_tree_view_column_new_with_attributes(
        "Credit ($)", creditRenderer, "text", 1, NULL)};
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), creditColumn);
    auto *dateRenderer{gtk_cell_renderer_text_new()};
    auto *dateColumn{gtk_tree_view_column_new_with_attributes(
        "Date (mm/dd/yyy)", dateRenderer, "text", 2, NULL)};
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), dateColumn);
    auto *descriptionRenderer{gtk_cell_renderer_text_new()};
    auto *descriptionColumn{gtk_tree_view_column_new_with_attributes(
        "Description", descriptionRenderer, "text", 3, NULL)};
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), descriptionColumn);
    for (const auto &transaction : transactions) {
      GtkTreeIter treeIter;
      gtk_tree_store_append(store, &treeIter, nullptr);
      std::stringstream stream;
      stream << transaction.verifiableTransaction.transaction.date;
      gtk_tree_store_set(
          store, &treeIter, 0,
          transaction.type == Transaction::Type::credit
              ? format(transaction.verifiableTransaction.transaction.amount)
                    .c_str()
              : "",
          1,
          transaction.type == Transaction::Type::debit
              ? format(transaction.verifiableTransaction.transaction.amount)
                    .c_str()
              : "",
          2, stream.str().c_str(), 3,
          transaction.verifiableTransaction.transaction.description.c_str(),
          -1);
    }
    g_object_unref(G_OBJECT(store));
    gtk_expander_set_child(GTK_EXPANDER(expander), tree);
    gtk_list_box_append(GTK_LIST_BOX(listBox), expander);
  }

private:
  GtkWidget *listBox;
};

class FileStreamFactory : public IoStreamFactory {
public:
  auto makeInput() -> std::shared_ptr<std::istream> override {
    return std::make_shared<std::ifstream>("/home/seth/budget.txt");
  }

  auto makeOutput() -> std::shared_ptr<std::ostream> override {
    return std::make_shared<std::ofstream>("/home/seth/budget.txt");
  }
};
} // namespace sbash64::budget

static void on_activate(GtkApplication *app) {
  auto *accountFactory{new sbash64::budget::InMemoryAccount::Factory};
  auto *bank{new sbash64::budget::Bank{*accountFactory}};
  auto *streamFactory{new sbash64::budget::FileStreamFactory};
  auto *serialization{
      new sbash64::budget::WritesSessionToStream{*streamFactory}};
  auto *deserialization{
      new sbash64::budget::ReadsSessionFromStream{*streamFactory}};
  bank->load(*deserialization);
  std::cout << "we loaded\n";
  auto *window{gtk_application_window_new(app)};
  auto *view{new sbash64::budget::GtkView(GTK_WINDOW(window))};
  bank->show(*view);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
  auto *app{gtk_application_new("com.example.GtkApplication",
                                G_APPLICATION_FLAGS_NONE)};
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  return g_application_run(G_APPLICATION(app), argc, argv);
}