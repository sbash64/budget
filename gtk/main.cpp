#include <fstream>
#include <gtk/gtk.h>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/command-line.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sstream>
#include <string>
#include <string_view>

namespace sbash64::budget {
static auto amountIf(const VerifiableTransactionWithType &transaction,
                     Transaction::Type type) -> std::string {
  return transaction.type == type
             ? format(transaction.verifiableTransaction.transaction.amount)
             : "";
}

class GtkView : public View {
public:
  explicit GtkView(Model &model, GtkWindow *window)
      : model{model}, accountsListBox{gtk_list_box_new()},
        transactionTypeComboBox{gtk_combo_box_text_new()},
        amountEntry{gtk_entry_new()}, calendar{gtk_calendar_new()} {
    auto *verticalBox{gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)};
    auto *scrolledWindow{gtk_scrolled_window_new()};
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(scrolledWindow), 300);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow),
                                  accountsListBox);
    gtk_box_append(GTK_BOX(verticalBox), scrolledWindow);
    auto *horizontalBox{gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8)};
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(transactionTypeComboBox),
                                   "Debit");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(transactionTypeComboBox),
                                   "Credit");
    gtk_combo_box_set_active(GTK_COMBO_BOX(transactionTypeComboBox), 0);
    gtk_widget_set_valign(transactionTypeComboBox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(horizontalBox), transactionTypeComboBox);
    gtk_entry_set_input_purpose(GTK_ENTRY(amountEntry),
                                GTK_INPUT_PURPOSE_NUMBER);
    gtk_widget_set_valign(amountEntry, GTK_ALIGN_CENTER);
    gtk_entry_set_placeholder_text(GTK_ENTRY(amountEntry), "0.00");
    gtk_box_append(GTK_BOX(horizontalBox), amountEntry);
    gtk_box_append(GTK_BOX(horizontalBox), calendar);
    auto *descriptionEntry{gtk_entry_new()};
    gtk_widget_set_valign(descriptionEntry, GTK_ALIGN_CENTER);
    gtk_entry_set_placeholder_text(GTK_ENTRY(descriptionEntry), "Description");
    gtk_box_append(GTK_BOX(horizontalBox), descriptionEntry);

    auto *addTransactionButton{gtk_button_new_with_label("Add")};
    g_signal_connect(addTransactionButton, "clicked",
                     G_CALLBACK(onAddTransaction), this);
    gtk_widget_set_valign(addTransactionButton, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(horizontalBox), addTransactionButton);
    gtk_box_append(GTK_BOX(verticalBox), horizontalBox);
    gtk_window_set_child(window, verticalBox);
  }

  void show(Account &primary,
            const std::vector<Account *> &secondaries) override {
    while (true) {
      auto *row{
          gtk_list_box_get_row_at_index(GTK_LIST_BOX(accountsListBox), 0)};
      if (row == nullptr)
        break;
      gtk_list_box_remove(GTK_LIST_BOX(accountsListBox), GTK_WIDGET(row));
    };
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
    gtk_list_box_append(GTK_LIST_BOX(accountsListBox), expander);
  }

private:
  static void onAddTransaction(GtkButton *, GtkView *view) {
    auto *transactionType{gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(view->transactionTypeComboBox))};
    auto *date{gtk_calendar_get_date(GTK_CALENDAR(view->calendar))};
    auto year{g_date_time_get_year(date)};
    auto month{g_date_time_get_month(date)};
    auto day{g_date_time_get_day_of_month(date)};
    if (std::string_view{transactionType} == "Debit") {
      view->model.debit(
          "idk", Transaction{usd(gtk_entry_buffer_get_text(gtk_entry_get_buffer(
                                 GTK_ENTRY(view->amountEntry)))),
                             "", Date{year, Month{month}, day}});
    } else {
      view->model.credit({});
    }
    view->model.show(*view);
    g_free(transactionType);
  }

  Model &model;
  GtkWidget *accountsListBox;
  GtkWidget *transactionTypeComboBox;
  GtkWidget *amountEntry;
  GtkWidget *calendar;
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
  auto *view{new GtkView{*bank, GTK_WINDOW(window)}};
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