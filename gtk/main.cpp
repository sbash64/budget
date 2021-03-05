#include <fstream>
#include <gtk/gtk.h>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/command-line.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sstream>
#include <string>

namespace sbash64::budget {
static auto amountIf(const VerifiableTransactionWithType &transaction,
                     Transaction::Type type) -> std::string {
  return transaction.type == type
             ? format(transaction.verifiableTransaction.transaction.amount)
             : "";
}

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
    std::stringstream titleStream;
    titleStream << name << ' ' << format(balance);
    auto *expander{gtk_expander_new(titleStream.str().c_str())};
    auto *store{gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_STRING)};
    auto *tree{gtk_tree_view_new_with_model(GTK_TREE_MODEL(store))};
    gtk_tree_view_append_column(
        GTK_TREE_VIEW(tree),
        gtk_tree_view_column_new_with_attributes(
            "Debit ($)", gtk_cell_renderer_text_new(), "text", 0, NULL));
    gtk_tree_view_append_column(
        GTK_TREE_VIEW(tree),
        gtk_tree_view_column_new_with_attributes(
            "Credit ($)", gtk_cell_renderer_text_new(), "text", 1, NULL));
    gtk_tree_view_append_column(
        GTK_TREE_VIEW(tree),
        gtk_tree_view_column_new_with_attributes(
            "Date (mm/dd/yyy)", gtk_cell_renderer_text_new(), "text", 2, NULL));
    gtk_tree_view_append_column(
        GTK_TREE_VIEW(tree),
        gtk_tree_view_column_new_with_attributes(
            "Description", gtk_cell_renderer_text_new(), "text", 3, NULL));
    for (const auto &transaction : transactions) {
      GtkTreeIter treeIter;
      gtk_tree_store_append(store, &treeIter, nullptr);
      std::stringstream dateStream;
      dateStream << transaction.verifiableTransaction.transaction.date;
      gtk_tree_store_set(
          store, &treeIter, 0,
          amountIf(transaction, Transaction::Type::debit).c_str(), 1,
          amountIf(transaction, Transaction::Type::credit).c_str(), 2,
          dateStream.str().c_str(), 3,
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

static void on_activate(GtkApplication *app) {
  auto *accountFactory{new InMemoryAccount::Factory};
  auto *bank{new Bank{*accountFactory}};
  auto *streamFactory{new FileStreamFactory};
  auto *serialization{new WritesSessionToStream{*streamFactory}};
  auto *deserialization{new ReadsSessionFromStream{*streamFactory}};
  bank->load(*deserialization);
  auto *window{gtk_application_window_new(app)};
  auto *view{new GtkView(GTK_WINDOW(window))};
  bank->show(*view);
  gtk_window_present(GTK_WINDOW(window));
}
} // namespace sbash64::budget

int main(int argc, char *argv[]) {
  auto *app{gtk_application_new("com.example.GtkApplication",
                                G_APPLICATION_FLAGS_NONE)};
  g_signal_connect(app, "activate", G_CALLBACK(sbash64::budget::on_activate),
                   NULL);
  return g_application_run(G_APPLICATION(app), argc, argv);
}