#include <fstream>
#include <gtk/gtk.h>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/command-line.hpp>
#include <sbash64/budget/serialization.hpp>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

#define TRANSACTION_TYPE_ITEM (transaction_item_get_type())
G_DECLARE_FINAL_TYPE(TransactionItem, transaction_item, TRANSACTION, ITEM,
                     GObject)

struct _TransactionItem {
  GObject parent_instance;
  GtkStringObject *debit_amount;
  GtkStringObject *credit_amount;
  GtkStringObject *date;
  GtkStringObject *description;
};

struct _TransactionItemClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(TransactionItem, transaction_item, G_TYPE_OBJECT)

static void transaction_item_init(TransactionItem *item) {}

static void transaction_item_class_init(TransactionItemClass *c) {}

static auto transaction_item_new(const std::string &debit_amount,
                                 const std::string &credit_amount,
                                 const std::string &date,
                                 const std::string &description)
    -> TransactionItem * {
  auto *item = static_cast<TransactionItem *>(
      g_object_new(TRANSACTION_TYPE_ITEM, nullptr));
  item->debit_amount = gtk_string_object_new(debit_amount.c_str());
  item->credit_amount = gtk_string_object_new(credit_amount.c_str());
  item->date = gtk_string_object_new(date.c_str());
  item->description = gtk_string_object_new(description.c_str());
  return item;
}

#define ACCOUNT_TYPE_ITEM (account_item_get_type())
G_DECLARE_FINAL_TYPE(AccountItem, account_item, ACCOUNT, ITEM, GObject)

struct _AccountItem {
  GObject parent_instance;
  GListStore *transactions;
  GtkStringObject *name;
  GtkStringObject *balance;
};

struct _AccountItemClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(AccountItem, account_item, G_TYPE_OBJECT)

static void account_item_init(AccountItem *item) {}

static void account_item_class_init(AccountItemClass *c) {}

static auto account_item_new(GListStore *transactions, const std::string &name,
                             const std::string &balance) -> AccountItem * {
  auto *item =
      static_cast<AccountItem *>(g_object_new(ACCOUNT_TYPE_ITEM, nullptr));
  item->transactions = transactions;
  item->name = gtk_string_object_new(name.c_str());
  item->balance = gtk_string_object_new(balance.c_str());
  return item;
}

static auto account_item_get_balance(AccountItem *item) -> const char * {
  return gtk_string_object_get_string(item->balance);
}

static auto account_item_get_name(AccountItem *item) -> const char * {
  return gtk_string_object_get_string(item->name);
}

static auto transaction_item_get_credit_amount(TransactionItem *item) -> const
    char * {
  return gtk_string_object_get_string(item->credit_amount);
}

static auto transaction_item_get_debit_amount(TransactionItem *item) -> const
    char * {
  return gtk_string_object_get_string(item->debit_amount);
}

static auto transaction_item_get_description(TransactionItem *item) -> const
    char * {
  return gtk_string_object_get_string(item->description);
}

static auto transaction_item_get_date(TransactionItem *item) -> const char * {
  return gtk_string_object_get_string(item->date);
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

static void setupExpander(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_list_item_set_child(list_item, gtk_expander_new(""));
}

static void bindCreditAmount(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      transaction_item_get_credit_amount(
          TRANSACTION_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))));
}

static void bindDebitAmount(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      transaction_item_get_debit_amount(
          TRANSACTION_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))));
}

static void bindDate(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      transaction_item_get_date(
          TRANSACTION_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))));
}

static void bindDescription(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      transaction_item_get_description(
          TRANSACTION_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))));
}

static void bindBalance(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      account_item_get_balance(
          ACCOUNT_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))));
}

static void setupAccountListItem(GtkListItemFactory *, GtkListItem *list_item) {
}

static void bindAccountListItem(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_list_item_set_child(
      list_item, static_cast<GtkWidget *>(gtk_list_item_get_item(list_item)));
}

static void bindAccount(GtkListItemFactory *, GtkListItem *list_item) {
  auto *const item{ACCOUNT_ITEM(gtk_list_item_get_item(list_item))};
  auto *const expander{gtk_list_item_get_child(list_item)};
  gtk_expander_set_label(GTK_EXPANDER(expander), account_item_get_name(item));

  auto *const columnView{gtk_column_view_new(GTK_SELECTION_MODEL(
      gtk_single_selection_new(G_LIST_MODEL(item->transactions))))};
  {
    auto *const signalListItemFactory{gtk_signal_list_item_factory_new()};
    g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                     NULL);
    g_signal_connect(signalListItemFactory, "bind", G_CALLBACK(bindDebitAmount),
                     NULL);
    gtk_column_view_append_column(
        GTK_COLUMN_VIEW(columnView),
        gtk_column_view_column_new("Debit ($)", signalListItemFactory));
  }
  {
    auto *const signalListItemFactory{gtk_signal_list_item_factory_new()};
    g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                     NULL);
    g_signal_connect(signalListItemFactory, "bind",
                     G_CALLBACK(bindCreditAmount), NULL);
    gtk_column_view_append_column(
        GTK_COLUMN_VIEW(columnView),
        gtk_column_view_column_new("Credit ($)", signalListItemFactory));
  }
  {
    auto *const signalListItemFactory{gtk_signal_list_item_factory_new()};
    g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                     NULL);
    g_signal_connect(signalListItemFactory, "bind", G_CALLBACK(bindDate), NULL);
    gtk_column_view_append_column(
        GTK_COLUMN_VIEW(columnView),
        gtk_column_view_column_new("Date (mm/dd/yyy)", signalListItemFactory));
  }
  {
    auto *const signalListItemFactory{gtk_signal_list_item_factory_new()};
    g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                     NULL);
    g_signal_connect(signalListItemFactory, "bind", G_CALLBACK(bindDescription),
                     NULL);
    gtk_column_view_append_column(
        GTK_COLUMN_VIEW(columnView),
        gtk_column_view_column_new("Description", signalListItemFactory));
  }
  gtk_expander_set_child(GTK_EXPANDER(expander), columnView);
}

constexpr const char *transactionTypeNames[]{"Debit", "Credit", nullptr};

class GtkView : public View {
public:
  explicit GtkView(Model &model, GtkWindow *window)
      : model{model}, accountListStore{g_list_store_new(G_TYPE_OBJECT)},
        accountSelection{
            gtk_single_selection_new(G_LIST_MODEL(accountListStore))},
        transactionTypeDropDown{gtk_drop_down_new_from_strings(
            static_cast<const char *const *>(transactionTypeNames))},
        amountEntry{gtk_entry_new()}, calendar{gtk_calendar_new()},
        descriptionEntry{gtk_entry_new()} {
    auto *const verticalBox{gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)};
    auto *const scrolledWindow{gtk_scrolled_window_new()};
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(scrolledWindow), 300);
    auto *const accountColumnView{
        gtk_column_view_new(GTK_SELECTION_MODEL(accountSelection))};
    {
      auto *const signalListItemFactory{gtk_signal_list_item_factory_new()};
      g_signal_connect(signalListItemFactory, "setup",
                       G_CALLBACK(setupExpander), NULL);
      g_signal_connect(signalListItemFactory, "bind", G_CALLBACK(bindAccount),
                       NULL);
      gtk_column_view_append_column(
          GTK_COLUMN_VIEW(accountColumnView),
          gtk_column_view_column_new("name", signalListItemFactory));
    }
    {
      auto *const signalListItemFactory{gtk_signal_list_item_factory_new()};
      g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setupLabel),
                       NULL);
      g_signal_connect(signalListItemFactory, "bind", G_CALLBACK(bindBalance),
                       NULL);
      gtk_column_view_append_column(
          GTK_COLUMN_VIEW(accountColumnView),
          gtk_column_view_column_new("balance ($)", signalListItemFactory));
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow),
                                  accountColumnView);
    gtk_box_append(GTK_BOX(verticalBox), scrolledWindow);
    auto *const horizontalBox{gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8)};
    gtk_drop_down_set_selected(GTK_DROP_DOWN(transactionTypeDropDown), 0);
    gtk_widget_set_valign(transactionTypeDropDown, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(horizontalBox), transactionTypeDropDown);
    gtk_entry_set_input_purpose(GTK_ENTRY(amountEntry),
                                GTK_INPUT_PURPOSE_NUMBER);
    gtk_widget_set_valign(amountEntry, GTK_ALIGN_CENTER);
    gtk_entry_set_placeholder_text(GTK_ENTRY(amountEntry), "0.00");
    gtk_box_append(GTK_BOX(horizontalBox), amountEntry);
    gtk_box_append(GTK_BOX(horizontalBox), calendar);
    gtk_widget_set_valign(descriptionEntry, GTK_ALIGN_CENTER);
    gtk_entry_set_placeholder_text(GTK_ENTRY(descriptionEntry), "Description");
    gtk_box_append(GTK_BOX(horizontalBox), descriptionEntry);

    auto *const addTransactionButton{gtk_button_new_with_label("Add")};
    g_signal_connect(addTransactionButton, "clicked",
                     G_CALLBACK(onAddTransaction), this);
    gtk_widget_set_valign(addTransactionButton, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(horizontalBox), addTransactionButton);
    gtk_box_append(GTK_BOX(verticalBox), horizontalBox);
    gtk_window_set_child(window, verticalBox);
  }

  void show(Account &primary,
            const std::vector<Account *> &secondaries) override {
    g_list_store_remove_all(accountListStore);
    primary.show(*this);
    for (auto *const secondary : secondaries)
      secondary->show(*this);
  }

  void showAccountSummary(
      std::string_view name, USD balance,
      const std::vector<VerifiableTransactionWithType> &transactions) override {
    auto *const transactionListStore{g_list_store_new(G_TYPE_OBJECT)};
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
    AccountItem *accountItem = account_item_new(
        transactionListStore, std::string{name}, format(balance));
    g_list_store_append(accountListStore, accountItem);
    g_object_unref(accountItem);
  }

private:
  static auto transaction(GtkView *view) -> Transaction {
    auto *const date{gtk_calendar_get_date(GTK_CALENDAR(view->calendar))};
    return {usd(gtk_entry_buffer_get_text(
                gtk_entry_get_buffer(GTK_ENTRY(view->amountEntry)))),
            gtk_entry_buffer_get_text(
                gtk_entry_get_buffer(GTK_ENTRY(view->descriptionEntry))),
            Date{g_date_time_get_year(date), Month{g_date_time_get_month(date)},
                 g_date_time_get_day_of_month(date)}};
  }

  static void onAddTransaction(GtkButton *, GtkView *view) {
    const auto *transactionType{transactionTypeNames[gtk_drop_down_get_selected(
        GTK_DROP_DOWN(view->transactionTypeDropDown))]};
    if (std::string_view{transactionType} == "Debit")
      view->model.debit(
          account_item_get_name((ACCOUNT_ITEM(g_list_model_get_item(
              G_LIST_MODEL(view->accountListStore),
              gtk_single_selection_get_selected(view->accountSelection))))),
          transaction(view));
    else if (std::string_view{transactionType} == "Credit")
      view->model.credit(transaction(view));
    view->model.show(*view);
  }

  Model &model;
  GListStore *accountListStore;
  GtkSingleSelection *accountSelection;
  GtkWidget *transactionTypeDropDown;
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