#include <gtk/gtk.h>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/command-line.hpp>

namespace sbash64::budget {
class GtkView : public View {
public:
  explicit GtkView(GtkWindow *window)
      : store{gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                 G_TYPE_STRING)} {
    auto *tree{gtk_tree_view_new_with_model(GTK_TREE_MODEL(store))};
    // g_object_unref (G_OBJECT (store));
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
    gtk_window_set_child(window, tree);
  }

  void show(Account &primary,
            const std::vector<Account *> &secondaries) override {}

  void showAccountSummary(
      std::string_view name, USD balance,
      const std::vector<VerifiableTransactionWithType> &transactions) override {
    for (const auto &transaction : transactions) {
      GtkTreeIter treeIter;
      gtk_tree_store_append(store, &treeIter, nullptr);
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
          2, "hi", 3,
          transaction.verifiableTransaction.transaction.description.c_str(),
          -1);
    }
  }

private:
  GtkTreeStore *store;
};
} // namespace sbash64::budget

static void on_activate(GtkApplication *app) {
  auto *window{gtk_application_window_new(app)};
  auto *view{new sbash64::budget::GtkView(GTK_WINDOW(window))};
  view->showAccountSummary(
      "Bob's", sbash64::budget::USD{15000},
      {debit(sbash64::budget::Transaction{
           sbash64::budget::USD{2435}, "Sam's 24th",
           sbash64::budget::Date{2020, sbash64::budget::Month::December, 27}}),
       credit(sbash64::budget::Transaction{
           sbash64::budget::USD{3511}, "Birthday present",
           sbash64::budget::Date{2021, sbash64::budget::Month::October, 20}}),
       verifiedDebit(sbash64::budget::Transaction{
           sbash64::budget::USD{2322}, "Hannah's 30th",
           sbash64::budget::Date{2021, sbash64::budget::Month::March, 8}})});
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
  auto *app{gtk_application_new("com.example.GtkApplication",
                                G_APPLICATION_FLAGS_NONE)};
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  return g_application_run(G_APPLICATION(app), argc, argv);
}