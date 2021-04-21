#include <fstream>
#include <gtk/gtk.h>
#include <memory>
#include <sbash64/budget/bank.hpp>
#include <sbash64/budget/budget.hpp>
#include <sbash64/budget/control.hpp>
#include <sbash64/budget/format.hpp>
#include <sbash64/budget/parse.hpp>
#include <sbash64/budget/serialization.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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
  g_object_unref(TRANSACTION_ITEM(object)->description);
  G_OBJECT_CLASS(transaction_item_parent_class)->finalize(object);
}

static void transaction_item_class_init(TransactionItemClass *c) {
  G_OBJECT_CLASS(c)->finalize = transaction_item_finalize;
}

#define ACCOUNT_TYPE_ITEM (account_item_get_type())
G_DECLARE_FINAL_TYPE(AccountItem, account_item, ACCOUNT, ITEM, GObject)

struct _AccountItem {
  GObject parent_instance;
  GListStore *transactionListStore;
  GtkStringObject *name;
  long long balanceCents;
};

struct _AccountItemClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(AccountItem, account_item, G_TYPE_OBJECT)

static void account_item_finalize(GObject *object) {
  const auto *accountItem{ACCOUNT_ITEM(object)};
  g_object_unref(accountItem->name);
  g_object_unref(accountItem->transactionListStore);
  G_OBJECT_CLASS(account_item_parent_class)->finalize(object);
}

static void account_item_init(AccountItem *item) {}

static void account_item_class_init(AccountItemClass *c) {
  G_OBJECT_CLASS(c)->finalize = account_item_finalize;
}
}

namespace sbash64::budget {
static void setupLabel(GtkListItemFactory *, GtkListItem *list_item) {
  gtk_list_item_set_child(list_item, gtk_label_new(""));
}

static void setLabel(GtkListItem *listItem, const char *string) {
  gtk_label_set_label(GTK_LABEL(gtk_list_item_get_child(listItem)), string);
}

static void bindCreditAmount(GtkListItemFactory *, GtkListItem *list_item) {
  auto *const transactionItem{
      TRANSACTION_ITEM(gtk_list_item_get_item(list_item))};
  if (transactionItem->credit) {
    std::stringstream stream;
    putWithDollarSign(stream, USD{transactionItem->cents});
    setLabel(list_item, stream.str().c_str());
  }
}

static void bindDebitAmount(GtkListItemFactory *, GtkListItem *list_item) {
  auto *const transactionItem{
      TRANSACTION_ITEM(gtk_list_item_get_item(list_item))};
  if (!transactionItem->credit) {
    std::stringstream stream;
    putWithDollarSign(stream, USD{transactionItem->cents});
    setLabel(list_item, stream.str().c_str());
  }
}

static void bindDate(GtkListItemFactory *, GtkListItem *listItem) {
  auto *const transactionItem{
      TRANSACTION_ITEM(gtk_list_item_get_item(listItem))};
  std::stringstream stream;
  stream << Date{transactionItem->year, Month{transactionItem->month},
                 transactionItem->day};
  setLabel(listItem, stream.str().c_str());
}

static void setLabel(GtkListItem *listItem, GtkStringObject *string) {
  setLabel(listItem, gtk_string_object_get_string(string));
}

static void bindDescription(GtkListItemFactory *, GtkListItem *listItem) {
  setLabel(listItem,
           TRANSACTION_ITEM(gtk_list_item_get_item(listItem))->description);
}

static void bindBalance(GtkListItemFactory *, GtkListItem *listItem) {
  std::stringstream stream;
  putWithDollarSign(
      stream,
      USD{ACCOUNT_ITEM(gtk_list_item_get_item(listItem))->balanceCents});
  setLabel(listItem, stream.str().c_str());
}

static void bindAccountName(GtkListItemFactory *, GtkListItem *listItem) {
  setLabel(listItem, ACCOUNT_ITEM(gtk_list_item_get_item(listItem))->name);
}

static auto transaction(TransactionItem *transactionItem) -> Transaction {
  return {USD{transactionItem->cents},
          gtk_string_object_get_string(transactionItem->description),
          Date{transactionItem->year, Month{transactionItem->month},
               transactionItem->day}};
}

static void selectionChanged(GtkSelectionModel *accountSelection, guint, guint,
                             gpointer transactionSelection) {
  gtk_single_selection_set_model(
      GTK_SINGLE_SELECTION(transactionSelection),
      G_LIST_MODEL(
          ACCOUNT_ITEM(g_list_model_get_item(
                           gtk_single_selection_get_model(
                               GTK_SINGLE_SELECTION(accountSelection)),
                           gtk_single_selection_get_selected(
                               GTK_SINGLE_SELECTION(accountSelection))))
              ->transactionListStore));
}

static void appendSignalGeneratedColumn(
    GtkWidget *columnView, void (*setup)(GtkListItemFactory *, GtkListItem *),
    void (*bind)(GtkListItemFactory *, GtkListItem *), const char *label) {
  auto *const signalListItemFactory{gtk_signal_list_item_factory_new()};
  g_signal_connect(signalListItemFactory, "setup", G_CALLBACK(setup), nullptr);
  g_signal_connect(signalListItemFactory, "bind", G_CALLBACK(bind), nullptr);
  gtk_column_view_append_column(
      GTK_COLUMN_VIEW(columnView),
      gtk_column_view_column_new(label, signalListItemFactory));
}

static auto sameTransaction(gconstpointer a, gconstpointer b) -> gboolean {
  const auto *first{static_cast<const TransactionItem *>(a)};
  const auto *second{static_cast<const TransactionItem *>(b)};
  return static_cast<gboolean>(
      first->cents == second->cents && first->credit == second->credit &&
      first->day == second->day && first->month == second->month &&
      first->year == second->year &&
      std::string_view{gtk_string_object_get_string(first->description)} ==
          gtk_string_object_get_string(second->description));
}

class GtkAccountView : public Account::Observer {
public:
  explicit GtkAccountView(GListStore *accountListStore,
                          AccountItem *accountItem)
      : accountListStore{accountListStore}, accountItem{accountItem} {
    g_list_store_append(accountListStore, accountItem);
  }

  void notifyThatBalanceHasChanged(USD balance) override {
    accountItem->balanceCents = balance.cents;
  }

  void notifyThatCreditHasBeenAdded(const Transaction &transaction) override {
    auto *const item = static_cast<TransactionItem *>(
        g_object_new(TRANSACTION_TYPE_ITEM, nullptr));
    item->credit = true;
    item->cents = transaction.amount.cents;
    item->year = transaction.date.year;
    item->month = static_cast<typename std::underlying_type<Month>::type>(
        transaction.date.month);
    item->day = transaction.date.day;
    item->description = gtk_string_object_new(transaction.description.c_str());
    g_list_store_append(accountItem->transactionListStore, item);
    g_object_unref(item);
  }

  void notifyThatDebitHasBeenAdded(const Transaction &transaction) override {
    auto *const item = static_cast<TransactionItem *>(
        g_object_new(TRANSACTION_TYPE_ITEM, nullptr));
    item->credit = false;
    item->cents = transaction.amount.cents;
    item->year = transaction.date.year;
    item->month = static_cast<typename std::underlying_type<Month>::type>(
        transaction.date.month);
    item->day = transaction.date.day;
    item->description = gtk_string_object_new(transaction.description.c_str());
    g_list_store_append(accountItem->transactionListStore, item);
    g_object_unref(item);
  }

  void notifyThatDebitHasBeenRemoved(const Transaction &transaction) override {
    auto *const item = static_cast<TransactionItem *>(
        g_object_new(TRANSACTION_TYPE_ITEM, nullptr));
    item->credit = false;
    item->cents = transaction.amount.cents;
    item->year = transaction.date.year;
    item->month = static_cast<typename std::underlying_type<Month>::type>(
        transaction.date.month);
    item->day = transaction.date.day;
    item->description = gtk_string_object_new(transaction.description.c_str());
    guint position = 0;
    g_list_store_find_with_equal_func(accountItem->transactionListStore, item,
                                      sameTransaction, &position);
    g_list_store_remove(accountItem->transactionListStore, position);
    g_object_unref(item);
  }

  void notifyThatCreditHasBeenRemoved(const Transaction &transaction) override {
    auto *const item = static_cast<TransactionItem *>(
        g_object_new(TRANSACTION_TYPE_ITEM, nullptr));
    item->credit = true;
    item->cents = transaction.amount.cents;
    item->year = transaction.date.year;
    item->month = static_cast<typename std::underlying_type<Month>::type>(
        transaction.date.month);
    item->day = transaction.date.day;
    item->description = gtk_string_object_new(transaction.description.c_str());
    guint position = 0;
    g_list_store_find_with_equal_func(accountItem->transactionListStore, item,
                                      sameTransaction, &position);
    g_list_store_remove(accountItem->transactionListStore, position);
    g_object_unref(item);
  }

  ~GtkAccountView() override {
    guint position = 0;
    g_list_store_find(accountListStore, accountItem, &position);
    g_list_store_remove(accountListStore, position);
  }

private:
  GListStore *accountListStore;
  AccountItem *accountItem;
};

static std::string globalOutputFilePath{"/home/seth/budget.txt"};

class GtkView : public Bank::Observer, public Control {
public:
  explicit GtkView(Model &model, SessionSerialization &serialization,
                   GtkWindow *window)
      : model{model}, serialization{serialization},
        accountListStore{g_list_store_new(ACCOUNT_TYPE_ITEM)},
        accountSelection{
            gtk_single_selection_new(G_LIST_MODEL(accountListStore))},
        transactionSelection{gtk_single_selection_new(
            G_LIST_MODEL(g_list_store_new(G_TYPE_OBJECT)))},
        window{window},
        amountEntry{gtk_entry_new()}, calendar{gtk_calendar_new()},
        descriptionEntry{gtk_entry_new()}, totalBalance{gtk_label_new("")} {
    model.attach(this);
    auto *const verticalBox{gtk_box_new(GTK_ORIENTATION_VERTICAL, 8)};
    auto *const scrolledWindowsPane{gtk_paned_new(GTK_ORIENTATION_HORIZONTAL)};
    auto *const leftHandScrolledWindow{gtk_scrolled_window_new()};
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(leftHandScrolledWindow), 300);
    auto *const accountsColumnView{
        gtk_column_view_new(GTK_SELECTION_MODEL(accountSelection))};
    appendSignalGeneratedColumn(accountsColumnView, setupLabel, bindAccountName,
                                "Name");
    appendSignalGeneratedColumn(accountsColumnView, setupLabel, bindBalance,
                                "Balance");
    gtk_widget_set_hexpand(leftHandScrolledWindow, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(leftHandScrolledWindow),
                                  accountsColumnView);
    gtk_paned_set_start_child(GTK_PANED(scrolledWindowsPane),
                              leftHandScrolledWindow);
    auto *const rightHandScrolledWindow{gtk_scrolled_window_new()};
    auto *const selectedAccountColumnView{
        gtk_column_view_new(GTK_SELECTION_MODEL(transactionSelection))};
    appendSignalGeneratedColumn(selectedAccountColumnView, setupLabel,
                                bindDebitAmount, "Debit");
    appendSignalGeneratedColumn(selectedAccountColumnView, setupLabel,
                                bindCreditAmount, "Credit");
    appendSignalGeneratedColumn(selectedAccountColumnView, setupLabel, bindDate,
                                "Date (mm/dd/yyy)");
    appendSignalGeneratedColumn(selectedAccountColumnView, setupLabel,
                                bindDescription, "Description");
    gtk_widget_set_hexpand(rightHandScrolledWindow, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(rightHandScrolledWindow),
                                  selectedAccountColumnView);
    gtk_paned_set_end_child(GTK_PANED(scrolledWindowsPane),
                            rightHandScrolledWindow);
    gtk_paned_set_position(GTK_PANED(scrolledWindowsPane), 250);
    g_signal_connect(accountSelection, "selection-changed",
                     G_CALLBACK(selectionChanged), transactionSelection);
    gtk_box_append(GTK_BOX(verticalBox), scrolledWindowsPane);
    gtk_box_append(GTK_BOX(verticalBox), totalBalance);
    auto *const transactionControlBox{
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8)};
    gtk_entry_set_input_purpose(GTK_ENTRY(amountEntry),
                                GTK_INPUT_PURPOSE_NUMBER);
    gtk_widget_set_valign(amountEntry, GTK_ALIGN_CENTER);
    gtk_entry_set_placeholder_text(GTK_ENTRY(amountEntry), "0.00");
    gtk_box_append(GTK_BOX(transactionControlBox), amountEntry);
    gtk_box_append(GTK_BOX(transactionControlBox), calendar);
    gtk_widget_set_valign(descriptionEntry, GTK_ALIGN_CENTER);
    gtk_entry_set_placeholder_text(GTK_ENTRY(descriptionEntry), "Description");
    gtk_box_append(GTK_BOX(transactionControlBox), descriptionEntry);
    addTransactionControlButton(this, transactionControlBox, "Add",
                                onAddTransaction);
    addTransactionControlButton(this, transactionControlBox, "Remove",
                                onRemoveTransaction);
    addTransactionControlButton(this, transactionControlBox, "Verify",
                                onVerifyTransaction);
    addTransactionControlButton(this, transactionControlBox, "Transfer To",
                                onTransferTo);
    addTransactionControlButton(this, transactionControlBox,
                                "Transfer To New Account",
                                onTransferToNewAccount);
    gtk_box_append(GTK_BOX(verticalBox), transactionControlBox);
    auto *const accountControlBox{gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8)};
    addTransactionControlButton(this, accountControlBox, "Remove",
                                onRemoveAccount);
    addTransactionControlButton(this, accountControlBox, "Reduce", onReduce);
    addTransactionControlButton(this, accountControlBox, "Save", onSave);
    addTransactionControlButton(this, accountControlBox, "Save As", onSaveAs);
    gtk_box_append(GTK_BOX(verticalBox), accountControlBox);
    gtk_window_set_child(window, verticalBox);
  }

  void notifyThatNewAccountHasBeenCreated(Account &account,
                                          std::string_view name) override {
    auto *const accountItem =
        static_cast<AccountItem *>(g_object_new(ACCOUNT_TYPE_ITEM, nullptr));
    accountItem->transactionListStore = g_list_store_new(TRANSACTION_TYPE_ITEM);
    accountItem->name = gtk_string_object_new(std::string{name}.c_str());
    accountItem->balanceCents = 0;
    auto accountView{
        std::make_unique<GtkAccountView>(accountListStore, accountItem)};
    account.attach(accountView.get());
    accountViews[std::string{name}] = std::move(accountView);
    g_object_unref(accountItem);
  }

  void notifyThatAccountHasBeenRemoved(std::string_view name) override {
    accountViews.erase(std::string{name});
  }

  void notifyThatTotalBalanceHasChanged(USD balance) override {
    std::stringstream stream;
    putWithDollarSign(stream, balance);
    gtk_label_set_label(GTK_LABEL(totalBalance), stream.str().c_str());
  }

  void attach(Control::Observer *a) override { observer = a; }

  auto year() -> int override {
    return g_date_time_get_year(gtk_calendar_get_date(GTK_CALENDAR(calendar)));
  }

  auto month() -> int override {
    return g_date_time_get_month(gtk_calendar_get_date(GTK_CALENDAR(calendar)));
  }

  auto day() -> int override {
    return g_date_time_get_day_of_month(
        gtk_calendar_get_date(GTK_CALENDAR(calendar)));
  }

  auto amountUsd() -> std::string override {
    return gtk_entry_buffer_get_text(
        gtk_entry_get_buffer(GTK_ENTRY(amountEntry)));
  }

  auto description() -> std::string override {
    return gtk_entry_buffer_get_text(
        gtk_entry_get_buffer(GTK_ENTRY(descriptionEntry)));
  }

  auto accountName() -> std::string override {
    return gtk_string_object_get_string(
        ACCOUNT_ITEM(g_list_model_get_item(
                         G_LIST_MODEL(accountListStore),
                         gtk_single_selection_get_selected(accountSelection)))
            ->name);
  }

  auto selectedTransaction() -> Transaction override {
    return budget::transaction(TRANSACTION_ITEM(g_list_model_get_item(
        gtk_single_selection_get_model(transactionSelection),
        gtk_single_selection_get_selected(transactionSelection))));
  }

  auto selectedTransactionIsCredit() -> bool override {
    return TRANSACTION_ITEM(
               g_list_model_get_item(
                   gtk_single_selection_get_model(transactionSelection),
                   gtk_single_selection_get_selected(transactionSelection)))
        ->credit;
  }

private:
  static void
  addTransactionControlButton(GtkView *view, GtkWidget *transactionControlBox,
                              const char *label,
                              void (*callback)(GtkButton *, GtkView *)) {
    auto *const button{gtk_button_new_with_label(label)};
    g_signal_connect(button, "clicked", G_CALLBACK(callback), view);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(transactionControlBox), button);
  }

  static void onAddTransaction(GtkButton *, GtkView *view) {
    view->observer->notifyThatAddTransactionButtonHasBeenPressed();
  }

  static void onRemoveTransaction(GtkButton *, GtkView *view) {
    view->observer->notifyThatRemoveTransactionButtonHasBeenPressed();
  }

  static void onRemoveAccount(GtkButton *, GtkView *view) {
    view->observer->notifyThatRemoveAccountButtonHasBeenPressed();
  }

  static void onVerifyTransaction(GtkButton *, GtkView *view) {
    view->observer->notifyThatVerifyTransactionButtonHasBeenPressed();
  }

  static void onTransferTo(GtkButton *, GtkView *view) {
    view->observer->notifyThatTransferToButtonHasBeenPressed();
  }

  static void onTransferToNewAccount(GtkButton *, GtkView *view) {
    view->observer->notifyThatTransferToNewAccountButtonHasBeenPressed();
  }

  static void on_save_response(GtkNativeDialog *dialog, int response,
                               GtkView *view) {
    if (response == GTK_RESPONSE_ACCEPT) {
      auto *const file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
      auto *path = g_file_get_path(file);
      globalOutputFilePath = path;
      g_free(path);
      g_object_unref(file);
      view->model.save(view->serialization);
    }
    g_object_unref(dialog);
  }

  static void onSaveAs(GtkButton *, GtkView *view) {
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    auto *const dialog = gtk_file_chooser_native_new(
        "Save File", view->window, action, "_Save", "_Cancel");
    g_signal_connect(dialog, "response", G_CALLBACK(on_save_response), view);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
  }

  static void onSave(GtkButton *, GtkView *view) {
    view->model.save(view->serialization);
  }

  static void onReduce(GtkButton *, GtkView *view) {
    view->observer->notifyThatReduceButtonHasBeenPressed();
  }

  std::map<std::string, std::unique_ptr<GtkAccountView>, std::less<>>
      accountViews;
  Model &model;
  SessionSerialization &serialization;
  GListStore *accountListStore;
  GtkSingleSelection *accountSelection;
  GtkSingleSelection *transactionSelection;
  GtkWindow *window;
  GtkWidget *amountEntry;
  GtkWidget *calendar;
  GtkWidget *descriptionEntry;
  GtkWidget *totalBalance;
  Control::Observer *observer{};
};

class FileStreamFactory : public IoStreamFactory {
public:
  auto makeInput() -> std::shared_ptr<std::istream> override {
    return std::make_shared<std::ifstream>("/home/seth/budget.txt");
  }

  auto makeOutput() -> std::shared_ptr<std::ostream> override {
    return std::make_shared<std::ofstream>(globalOutputFilePath);
  }
};

static void on_activate(GtkApplication *app) {
  static InMemoryAccount::Factory accountFactory;
  static Bank bank{accountFactory};
  static FileStreamFactory streamFactory;
  static WritesAccountToStream::Factory accountSerializationFactory;
  static WritesSessionToStream sessionSerialization{
      streamFactory, accountSerializationFactory};
  static ReadsAccountFromStream::Factory accountDeserializationFactory;
  static ReadsSessionFromStream accountDeserialization{
      streamFactory, accountDeserializationFactory};
  auto *window{gtk_application_window_new(app)};
  static GtkView view{bank, sessionSerialization, GTK_WINDOW(window)};
  static Controller controller{bank, view};
  bank.load(accountDeserialization);
  gtk_window_present(GTK_WINDOW(window));
}
} // namespace sbash64::budget

int main(int argc, char *argv[]) {
  auto *const app{
      gtk_application_new("com.sbash64.GtkBudget", G_APPLICATION_FLAGS_NONE)};
  g_signal_connect(app, "activate", G_CALLBACK(sbash64::budget::on_activate),
                   NULL);
  const auto status{g_application_run(G_APPLICATION(app), argc, argv)};
  g_object_unref(app);
  return status;
}
