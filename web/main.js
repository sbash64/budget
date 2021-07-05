function createChild(parent, tagName) {
  const child = document.createElement(tagName);
  parent.append(child);
  return child;
}

function initializeAccountTable(accountTable) {
  const header = createChild(accountTable, "tr");
  createChild(header, "th");
  createChild(header, "th").textContent = "Name";
  createChild(header, "th").textContent = "Balance";
}

function main() {
  const accountsWithSummaries = createChild(document.body, "div");
  accountsWithSummaries.style.display = "flex";
  const accountSummaryTable = createChild(accountsWithSummaries, "table");
  const totalBalance = createChild(document.body, "div");
  const controls = createChild(document.body, "div");
  controls.style.display = "flex";
  const descriptionLabel = createChild(controls, "label");
  descriptionLabel.textContent = "description";
  const descriptionInput = createChild(descriptionLabel, "input");
  descriptionInput.type = "text";
  const amountLabel = createChild(controls, "label");
  amountLabel.textContent = "amount";
  const amountInput = createChild(amountLabel, "input");
  amountInput.type = "number";
  amountInput.min = 0;
  amountInput.step = "any";
  const dateLabel = createChild(controls, "label");
  dateLabel.textContent = "date";
  const dateInput = createChild(dateLabel, "input");
  dateInput.type = "date";
  const addTransactionButton = createChild(controls, "button");
  addTransactionButton.textContent = "add";
  const removeTransactionButton = createChild(controls, "button");
  removeTransactionButton.textContent = "remove";
  initializeAccountTable(accountSummaryTable);
  let selectedAccountTable = null;
  let selectedTransactionRow = null;
  const accounts = new Map();
  const accountSummaries = new Map();
  const accountNames = new Map();
  const websocket = new WebSocket("ws://localhost:9012");
  websocket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    switch (message.method) {
      case "add account": {
        const accountSummary = createChild(accountSummaryTable, "tr");
        const selection = createChild(
          createChild(accountSummary, "td"),
          "input"
        );
        selection.name = "account selection";
        selection.type = "radio";
        const name = createChild(accountSummary, "td");
        name.textContent = message.name;
        createChild(accountSummary, "td");
        accountSummaries.set(message.name, accountSummary);

        const accountTable = createChild(accountsWithSummaries, "table");
        const header = createChild(accountTable, "tr");
        createChild(header, "th");
        createChild(header, "th").textContent = "Description";
        createChild(header, "th").textContent = "Debits";
        createChild(header, "th").textContent = "Credits";
        createChild(header, "th").textContent = "Date";
        accounts.set(message.name, accountTable);
        accountNames.set(accountTable, message.name);

        accountTable.style.display = "none";
        selection.addEventListener("change", () => {
          if (selectedAccountTable) selectedAccountTable.style.display = "none";
          accountTable.style.display = "block";
          selectedAccountTable = accountTable;
        });
        break;
      }
      case "remove account": {
        const account = accounts.get(message.name);
        account.parentNode.removeChild(account);
        accounts.delete(message.name);
        break;
      }
      case "update balance": {
        totalBalance.textContent = message.amount;
        break;
      }
      case "add credit": {
        const transaction = createChild(accounts.get(message.name), "tr");
        const selection = createChild(createChild(transaction, "td"), "input");
        selection.name = "transaction selection";
        selection.type = "radio";
        createChild(transaction, "td").textContent = message.description;
        createChild(transaction, "td");
        createChild(transaction, "td").textContent = message.amount;
        createChild(transaction, "td").textContent = message.date;
        selection.addEventListener("change", () => {
          selectedTransactionRow = transaction;
        });
        break;
      }
      case "remove credit": {
        const account = accounts.get(message.name);
        for (let i = 0; i < account.rows.length; i += 1) {
          if (
            account.rows[i].cells[1].textContent === message.description &&
            account.rows[i].cells[3].textContent === message.amount &&
            account.rows[i].cells[4].textContent === message.date
          ) {
            account.deleteRow(i);
            break;
          }
        }
        break;
      }
      case "add debit": {
        const transaction = createChild(accounts.get(message.name), "tr");
        const selection = createChild(createChild(transaction, "td"), "input");
        selection.name = "transaction selection";
        selection.type = "radio";
        createChild(transaction, "td").textContent = message.description;
        createChild(transaction, "td").textContent = message.amount;
        createChild(transaction, "td");
        createChild(transaction, "td").textContent = message.date;
        selection.addEventListener("change", () => {
          selectedTransactionRow = transaction;
        });
        break;
      }
      case "remove debit": {
        const account = accounts.get(message.name);
        for (let i = 0; i < account.rows.length; i += 1) {
          if (
            account.rows[i].cells[1].textContent === message.description &&
            account.rows[i].cells[2].textContent === message.amount &&
            account.rows[i].cells[4].textContent === message.date
          ) {
            account.deleteRow(i);
            break;
          }
        }
        break;
      }
      case "update account balance": {
        const accountSummary = accountSummaries.get(message.name);
        accountSummary.lastElementChild.textContent = message.amount;
        break;
      }
      default:
        break;
    }
  };
  addTransactionButton.addEventListener("click", () => {
    websocket.send(
      JSON.stringify({
        method: "debit",
        name: accountNames.get(selectedAccountTable),
        description: descriptionInput.value,
        amount: amountInput.value,
        date: dateInput.value,
      })
    );
  });
  removeTransactionButton.addEventListener("click", () => {
    websocket.send(
      JSON.stringify({
        method: "remove debit",
        name: accountNames.get(selectedAccountTable),
        description: selectedTransactionRow.cells[1].textContent,
        amount: selectedTransactionRow.cells[2].textContent,
        date: selectedTransactionRow.cells[4].textContent,
      })
    );
  });
}

document.addEventListener("DOMContentLoaded", () => {
  main();
});
