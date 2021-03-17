#include <fstream>
#include <gtk/gtk.h>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/command-line.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sstream>
#include <string>
#include <string_view>

extern "C" {
#define TRANSACTION_TYPE_ITEM (transaction_item_get_type())
G_DECLARE_FINAL_TYPE(TransactionItem, transaction_item, TRANSACTION, ITEM,
                     GObject)

struct _TransactionItem {
  GObject parent_instance;
  GtkStringObject *description;
  long long cents;
  int year;
  int month;
  int day;
  bool credit;
};

struct _TransactionItemClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(TransactionItem, transaction_item, G_TYPE_OBJECT)

static void transaction_item_init(TransactionItem *item) {}

static void transaction_item_finalize(GObject *object) {
  const auto *transactionItem{TRANSACTION_ITEM(object)};
  g_object_unref(transactionItem->description);
  G_OBJECT_CLASS(transaction_item_parent_class)->finalize(object);
}

static void transaction_item_class_init(TransactionItemClass *c) {
  auto *gobject_class = G_OBJECT_CLASS(c);
  gobject_class->finalize = transaction_item_finalize;
}

#define ACCOUNT_TYPE_ITEM (account_item_get_type())
G_DECLARE_FINAL_TYPE(AccountItem, account_item, ACCOUNT, ITEM, GObject)

struct _AccountItem {
  GObject parent_instance;
  GtkSingleSelection *transactionSelection;
  GtkStringObject *name;
  GtkStringObject *balance;
};

struct _AccountItemClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(AccountItem, account_item, G_TYPE_OBJECT)

static void account_item_finalize(GObject *object) {
  const auto *accountItem{ACCOUNT_ITEM(object)};
  g_object_unref(accountItem->balance);
  g_object_unref(accountItem->name);
  g_object_unref(accountItem->transactionSelection);
  G_OBJECT_CLASS(account_item_parent_class)->finalize(object);
}

static void account_item_init(AccountItem *item) {}

static void account_item_class_init(AccountItemClass *c) {
  auto *gobject_class = G_OBJECT_CLASS(c);
  gobject_class->finalize = account_item_finalize;
}
}

namespace sbash64::budget {
static void setupLabel(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_list_item_set_child(list_item, gtk_label_new(""));
}

static void bindCreditAmount(GtkListItemFactory *, GtkListItem *list_item) {
  auto *transactionItem{
      TRANSACTION_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))};
  if (transactionItem->credit)
    gtk_label_set_label(
        GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
        format(USD{transactionItem->cents}).c_str());
}

static void bindDebitAmount(GtkListItemFactory *, GtkListItem *list_item) {
  auto *transactionItem{
      TRANSACTION_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))};
  if (!transactionItem->credit)
    gtk_label_set_label(
        GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
        format(USD{transactionItem->cents}).c_str());
}

static void bindDate(GtkListItemFactory *, GtkListItem *list_item) {
  auto *transactionItem{
      TRANSACTION_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))};
  std::stringstream stream;
  stream << Date{transactionItem->year, Month{transactionItem->month},
                 transactionItem->day};
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      stream.str().c_str());
}

static void bindDescription(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      gtk_string_object_get_string(
          TRANSACTION_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))
              ->description));
}

static void bindBalance(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_label_set_label(
      GTK_LABEL(gtk_list_item_get_child(GTK_LIST_ITEM(list_item))),
      gtk_string_object_get_string(
          ACCOUNT_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)))
              ->balance));
}

static void setupExpandingAccount(GtkListItemFactory *,
                                  GtkListItem *list_item) {
  auto *const expander{gtk_expander_new("")};
  auto *const columnView{
      gtk_column_view_new(GTK_SELECTION_MODEL(gtk_single_selection_new(
          G_LIST_MODEL(g_list_store_new(G_TYPE_OBJECT)))))};
  {
    auto *const factory{gtk_signal_list_item_factory_new()};
    g_signal_connect(factory, "setup", G_CALLBACK(setupLabel), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bindDebitAmount), NULL);
    gtk_column_view_append_column(
        GTK_COLUMN_VIEW(columnView),
        gtk_column_view_column_new("Debit ($)", factory));
  }
  {
    auto *const factory{gtk_signal_list_item_factory_new()};
    g_signal_connect(factory, "setup", G_CALLBACK(setupLabel), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bindCreditAmount), NULL);
    gtk_column_view_append_column(
        GTK_COLUMN_VIEW(columnView),
        gtk_column_view_column_new("Credit ($)", factory));
  }
  {
    auto *const factory{gtk_signal_list_item_factory_new()};
    g_signal_connect(factory, "setup", G_CALLBACK(setupLabel), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bindDate), NULL);
    gtk_column_view_append_column(
        GTK_COLUMN_VIEW(columnView),
        gtk_column_view_column_new("Date (mm/dd/yyy)", factory));
  }
  {
    auto *const factory{gtk_signal_list_item_factory_new()};
    g_signal_connect(factory, "setup", G_CALLBACK(setupLabel), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bindDescription), NULL);
    gtk_column_view_append_column(
        GTK_COLUMN_VIEW(columnView),
        gtk_column_view_column_new("Description", factory));
  }
  gtk_expander_set_child(GTK_EXPANDER(expander), columnView);
  gtk_list_item_set_child(list_item, expander);
}

static void bindAccount(GtkListItemFactory *, GtkListItem *listItem) {
  auto *const accountItem{ACCOUNT_ITEM(gtk_list_item_get_item(listItem))};
  auto *const expander{gtk_list_item_get_child(listItem)};
  gtk_expander_set_label(GTK_EXPANDER(expander),
                         gtk_string_object_get_string(accountItem->name));
  gtk_column_view_set_model(
      GTK_COLUMN_VIEW(gtk_expander_get_child(GTK_EXPANDER(expander))),
      GTK_SELECTION_MODEL(accountItem->transactionSelection));
}

static auto transaction(TransactionItem *transactionItem) -> Transaction {
  return {USD{transactionItem->cents},
          gtk_string_object_get_string(transactionItem->description),
          Date{transactionItem->year, Month{transactionItem->month},
               transactionItem->day}};
}

class GtkView : public View {
public:
  explicit GtkView(Model &model, GtkWindow *window)
      : model{model}, accountListStore{g_list_store_new(ACCOUNT_TYPE_ITEM)},
        accountSelection{
            gtk_single_selection_new(G_LIST_MODEL(accountListStore))},
        amountEntry{gtk_entry_new()}, calendar{gtk_calendar_new()},
        descriptionEntry{gtk_entry_new()} {
    auto *const verticalBox{gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)};
    auto *const scrolledWindow{gtk_scrolled_window_new()};
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(scrolledWindow), 300);
    auto *const accountColumnView{
        gtk_column_view_new(GTK_SELECTION_MODEL(accountSelection))};
    {
      auto *const factory{gtk_signal_list_item_factory_new()};
      g_signal_connect(factory, "setup", G_CALLBACK(setupExpandingAccount),
                       NULL);
      g_signal_connect(factory, "bind", G_CALLBACK(bindAccount), NULL);
      gtk_column_view_append_column(
          GTK_COLUMN_VIEW(accountColumnView),
          gtk_column_view_column_new("name", factory));
    }
    {
      auto *const factory{gtk_signal_list_item_factory_new()};
      g_signal_connect(factory, "setup", G_CALLBACK(setupLabel), NULL);
      g_signal_connect(factory, "bind", G_CALLBACK(bindBalance), NULL);
      gtk_column_view_append_column(
          GTK_COLUMN_VIEW(accountColumnView),
          gtk_column_view_column_new("balance ($)", factory));
    }
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow),
                                  accountColumnView);
    gtk_box_append(GTK_BOX(verticalBox), scrolledWindow);
    auto *const horizontalBox{gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8)};
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
    auto *const removeTransactionButton{gtk_button_new_with_label("Remove")};
    g_signal_connect(removeTransactionButton, "clicked",
                     G_CALLBACK(onRemoveTransaction), this);
    gtk_widget_set_valign(removeTransactionButton, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(horizontalBox), removeTransactionButton);
    auto *const verifyTransactionButton{gtk_button_new_with_label("Verify")};
    g_signal_connect(verifyTransactionButton, "clicked",
                     G_CALLBACK(onVerifyTransaction), this);
    gtk_widget_set_valign(verifyTransactionButton, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(horizontalBox), verifyTransactionButton);
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
    auto *const transactionListStore{g_list_store_new(TRANSACTION_TYPE_ITEM)};
    for (const auto &transaction : transactions) {
      auto *const item = static_cast<TransactionItem *>(
          g_object_new(TRANSACTION_TYPE_ITEM, nullptr));
      item->credit = transaction.type == Transaction::Type::credit;
      item->cents = transaction.verifiableTransaction.transaction.amount.cents;
      item->year = transaction.verifiableTransaction.transaction.date.year;
      item->month = static_cast<typename std::underlying_type<Month>::type>(
          transaction.verifiableTransaction.transaction.date.month);
      item->day = transaction.verifiableTransaction.transaction.date.day;
      item->description = gtk_string_object_new(
          transaction.verifiableTransaction.transaction.description.c_str());
      g_list_store_append(transactionListStore, item);
      g_object_unref(item);
    }
    auto *const accountItem =
        static_cast<AccountItem *>(g_object_new(ACCOUNT_TYPE_ITEM, nullptr));
    accountItem->transactionSelection =
        gtk_single_selection_new(G_LIST_MODEL(transactionListStore));
    accountItem->name = gtk_string_object_new(std::string{name}.c_str());
    accountItem->balance = gtk_string_object_new(format(balance).c_str());
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

  static auto selectedAccountName(GtkView *view) -> const char * {
    return gtk_string_object_get_string(
        ACCOUNT_ITEM(g_list_model_get_item(G_LIST_MODEL(view->accountListStore),
                                           gtk_single_selection_get_selected(
                                               view->accountSelection)))
            ->name);
  }

  static void onAddTransaction(GtkButton *, GtkView *view) {
    if (std::string_view{selectedAccountName(view)} == "master")
      view->model.credit(transaction(view));
    else
      view->model.debit(selectedAccountName(view), transaction(view));
    view->model.show(*view);
  }

  static void onRemoveTransaction(GtkButton *, GtkView *view) {
    auto *const transactionSelection{
        ACCOUNT_ITEM(g_list_model_get_item(G_LIST_MODEL(view->accountListStore),
                                           gtk_single_selection_get_selected(
                                               view->accountSelection)))
            ->transactionSelection};
    auto *const transactionItem{TRANSACTION_ITEM(g_list_model_get_item(
        gtk_single_selection_get_model(transactionSelection),
        gtk_single_selection_get_selected(transactionSelection)))};
    if (transactionItem->credit &&
        std::string_view{selectedAccountName(view)} == "master")
      view->model.removeCredit(budget::transaction(transactionItem));
    else if (!transactionItem->credit &&
             std::string_view{selectedAccountName(view)} != "master")
      view->model.removeDebit(selectedAccountName(view),
                              budget::transaction(transactionItem));
    else
      return;
    view->model.show(*view);
  }

  static void onVerifyTransaction(GtkButton *, GtkView *view) {
    auto *const transactionSelection{
        ACCOUNT_ITEM(g_list_model_get_item(G_LIST_MODEL(view->accountListStore),
                                           gtk_single_selection_get_selected(
                                               view->accountSelection)))
            ->transactionSelection};
    auto *const transactionItem{TRANSACTION_ITEM(g_list_model_get_item(
        gtk_single_selection_get_model(transactionSelection),
        gtk_single_selection_get_selected(transactionSelection)))};
    if (transactionItem->credit &&
        std::string_view{selectedAccountName(view)} == "master")
      view->model.verifyCredit(budget::transaction(transactionItem));
    else if (!transactionItem->credit &&
             std::string_view{selectedAccountName(view)} != "master")
      view->model.verifyDebit(selectedAccountName(view),
                              budget::transaction(transactionItem));
    else
      return;
    view->model.show(*view);
  }

  Model &model;
  GListStore *accountListStore;
  GtkSingleSelection *accountSelection;
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
  static InMemoryAccount::Factory accountFactory;
  static Bank bank{accountFactory};
  static FileStreamFactory streamFactory;
  static WritesSessionToStream serialization{streamFactory};
  static ReadsSessionFromStream deserialization{streamFactory};
  bank.load(deserialization);
  auto *window{gtk_application_window_new(app)};
  static GtkView view{bank, GTK_WINDOW(window)};
  bank.show(view);
  gtk_window_present(GTK_WINDOW(window));
}
} // namespace sbash64::budget

int main(int argc, char *argv[]) {
  auto *const app{
      gtk_application_new("com.sbash64.GtkBudget", G_APPLICATION_FLAGS_NONE)};
  g_signal_connect(app, "activate", G_CALLBACK(sbash64::budget::on_activate),
                   NULL);
  return g_application_run(G_APPLICATION(app), argc, argv);
}