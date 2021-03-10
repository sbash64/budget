#include <fstream>
#include <gtk/gtk.h>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/command-line.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sstream>
#include <string>
#include <string_view>

#define TRANSACTION_TYPE_ITEM (transaction_item_get_type())
G_DECLARE_FINAL_TYPE(TransactionItem, transaction_item, TRANSACTION, ITEM,
                     GObject)

struct _TransactionItem {
  GObject parent_instance;
  std::string debit_amount;
  std::string credit_amount;
  std::string date;
  std::string description;
};

struct _TransactionItemClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(TransactionItem, transaction_item, G_TYPE_OBJECT)

static void transaction_item_init(TransactionItem *item) {}

static void transaction_item_class_init(TransactionItemClass *c) {}

static auto transaction_item_new(std::string_view debit_amount,
                                 std::string_view credit_amount,
                                 std::string_view date,
                                 std::string_view description)
    -> TransactionItem * {
  auto *item = static_cast<TransactionItem *>(
      g_object_new(TRANSACTION_TYPE_ITEM, nullptr));
  item->debit_amount = debit_amount;
  item->credit_amount = credit_amount;
  item->date = date;
  item->description = description;
  return item;
}

static auto transaction_item_get_credit_amount(TransactionItem *item) -> const
    char * {
  return item->credit_amount.c_str();
}

static auto transaction_item_get_debit_amount(TransactionItem *item) -> const
    char * {
  return item->debit_amount.c_str();
}

static auto transaction_item_get_description(TransactionItem *item) -> const
    char * {
  return item->description.c_str();
}

static auto transaction_item_get_date(TransactionItem *item) -> const char * {
  return item->date.c_str();
}

namespace sbash64::budget {
static auto amountIf(const VerifiableTransactionWithType &transaction,
                     Transaction::Type type) -> std::string {
  return transaction.type == type
             ? format(transaction.verifiableTransaction.transaction.amount)
             : "";
}

static void setupLabel(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_list_item_set_child(list_item, gtk_label_new(""));
}

static void setupAccountListItem(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_list_item_set_child(list_item, gtk_label_new("hello"));
}

static void bindCreditAmount(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      transaction_item_get_credit_amount(
          TRANSACTION_ITEM(static_cast<GObject *>(
              gtk_list_item_get_item(GTK_LIST_ITEM(list_item))))));
}

static void bindDebitAmount(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      transaction_item_get_debit_amount(TRANSACTION_ITEM(static_cast<GObject *>(
          gtk_list_item_get_item(GTK_LIST_ITEM(list_item))))));
}

static void bindDate(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      transaction_item_get_date(TRANSACTION_ITEM(static_cast<GObject *>(
          gtk_list_item_get_item(GTK_LIST_ITEM(list_item))))));
}

static void bindDescription(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      transaction_item_get_description(TRANSACTION_ITEM(static_cast<GObject *>(
          gtk_list_item_get_item(GTK_LIST_ITEM(list_item))))));
}

// static void bindAccountListItem(GtkListItemFactory *factory,
//                                 GtkListItem *list_item) {
//   GtkWidget *image;
//   GAppInfo *app_info;

//   image = gtk_list_item_get_child(list_item);
//   app_info = gtk_list_item_get_item(list_item);
//   gtk_image_set_from_gicon(GTK_IMAGE(image), g_app_info_get_icon(app_info));
// }

class GtkView : public View {
public:
  explicit GtkView(Model &model, GtkWindow *window)
      : model{model}, accountsListBox{gtk_list_box_new()},
        accountListStore{g_list_store_new(G_TYPE_OBJECT)},
        transactionTypeComboBox{gtk_combo_box_text_new()},
        amountEntry{gtk_entry_new()}, calendar{gtk_calendar_new()},
        descriptionEntry{gtk_entry_new()} {
    auto *verticalBox{gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)};
    auto *scrolledWindow{gtk_scrolled_window_new()};
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(scrolledWindow), 300);

    // auto *accountListFactory = gtk_signal_list_item_factory_new();
    // g_signal_connect(accountListFactory, "setup",
    //                  G_CALLBACK(setupAccountListItem), NULL);
    // g_signal_connect(accountListFactory, "bind",
    //                  G_CALLBACK(bindAccountListItem), NULL);

    // auto *accountList =
    //     gtk_list_view_new(GTK_SELECTION_MODEL(gtk_single_selection_new(
    //                           G_LIST_MODEL(accountListStore))),
    //                       accountListFactory);

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
    // g_list_store_remove_all(accountListStore);
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
    auto *horizontalBox{gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8)};
    auto *accountNameLabel{gtk_label_new(std::string{name}.c_str())};
    gtk_box_append(GTK_BOX(horizontalBox), accountNameLabel);
    auto *balanceLabel{gtk_label_new(format(balance).c_str())};
    gtk_box_append(GTK_BOX(horizontalBox), balanceLabel);
    auto *expander{gtk_expander_new("")};
    gtk_expander_set_label_widget(GTK_EXPANDER(expander), horizontalBox);

    auto *transactionListStore{g_list_store_new(G_TYPE_OBJECT)};
    // auto *transactionTreeListModel{
    //     gtk_tree_list_model_new(G_LIST_MODEL(transactionListStore), FALSE,
    //                             FALSE, create_function, nullptr, nullptr)};
    auto *columnView{gtk_column_view_new(GTK_SELECTION_MODEL(
        gtk_single_selection_new(G_LIST_MODEL(transactionListStore))))};
    {
      auto *signalListItemFactory{gtk_signal_list_item_factory_new()};
      g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                       NULL);
      g_signal_connect(signalListItemFactory, "bind",
                       G_CALLBACK(bindDebitAmount), NULL);
      gtk_column_view_append_column(
          GTK_COLUMN_VIEW(columnView),
          gtk_column_view_column_new("Debit ($)", signalListItemFactory));
    }

    {
      auto *signalListItemFactory{gtk_signal_list_item_factory_new()};
      g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                       NULL);
      g_signal_connect(signalListItemFactory, "bind",
                       G_CALLBACK(bindCreditAmount), NULL);
      gtk_column_view_append_column(
          GTK_COLUMN_VIEW(columnView),
          gtk_column_view_column_new("Credit ($)", signalListItemFactory));
    }

    {
      auto *signalListItemFactory{gtk_signal_list_item_factory_new()};
      g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                       NULL);
      g_signal_connect(signalListItemFactory, "bind", G_CALLBACK(bindDate),
                       NULL);
      gtk_column_view_append_column(
          GTK_COLUMN_VIEW(columnView),
          gtk_column_view_column_new("Date (mm/dd/yyy)",
                                     signalListItemFactory));
    }

    {
      auto *signalListItemFactory{gtk_signal_list_item_factory_new()};
      g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                       NULL);
      g_signal_connect(signalListItemFactory, "bind",
                       G_CALLBACK(bindDescription), NULL);
      gtk_column_view_append_column(
          GTK_COLUMN_VIEW(columnView),
          gtk_column_view_column_new("Description", signalListItemFactory));
    }
    // auto *treeExpander{gtk_tree_expander_new()};
    for (const auto &transaction : transactions) {
      std::stringstream dateStream;
      dateStream << transaction.verifiableTransaction.transaction.date;
      TransactionItem *item = transaction_item_new(
          amountIf(transaction, Transaction::Type::debit),
          amountIf(transaction, Transaction::Type::credit), dateStream.str(),
          transaction.verifiableTransaction.transaction.description);
      g_list_store_append(transactionListStore, item);
      g_object_unref(item);
    }
    gtk_expander_set_child(GTK_EXPANDER(expander), columnView);
    gtk_list_box_append(GTK_LIST_BOX(accountsListBox), expander);
  }

private:
  static auto transaction(GtkView *view) -> Transaction {
    auto *date{gtk_calendar_get_date(GTK_CALENDAR(view->calendar))};
    return Transaction{
        usd(gtk_entry_buffer_get_text(
            gtk_entry_get_buffer(GTK_ENTRY(view->amountEntry)))),
        gtk_entry_buffer_get_text(
            gtk_entry_get_buffer(GTK_ENTRY(view->descriptionEntry))),
        Date{g_date_time_get_year(date), Month{g_date_time_get_month(date)},
             g_date_time_get_day_of_month(date)}};
  }

  static void onAddTransaction(GtkButton *, GtkView *view) {
    auto *transactionType{gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(view->transactionTypeComboBox))};
    auto *selectedAccountListBoxRow{
        gtk_list_box_get_selected_row(GTK_LIST_BOX(view->accountsListBox))};
    auto *selectedAccountExpander{
        gtk_list_box_row_get_child(selectedAccountListBoxRow)};
    auto *selectedAccountHorizontalBox{
        gtk_expander_get_label_widget(GTK_EXPANDER(selectedAccountExpander))};
    auto *selectedAccountNameLabel{
        gtk_widget_get_first_child(selectedAccountHorizontalBox)};
    if (std::string_view{transactionType} == "Debit")
      view->model.debit(
          gtk_label_get_label(GTK_LABEL(selectedAccountNameLabel)),
          transaction(view));
    else if (std::string_view{transactionType} == "Credit")
      view->model.credit(transaction(view));
    view->model.show(*view);
    g_free(transactionType);
  }

  Model &model;
  GtkWidget *accountsListBox;
  GListStore *accountListStore;
  GtkWidget *transactionTypeComboBox;
  GtkWidget *amountEntry;
  GtkWidget *calendar;
  GtkWidget *descriptionEntry;
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