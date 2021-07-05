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

function transactionRowIsCredit(row) {
  return row.cells[3].textContent.length !== 0;
}

function accountTableIsMaster(selectedAccountTable, accountNames) {
  return accountNames.get(selectedAccountTable) !== "master";
}

class CreditControl {
  name() {
    return "credit";
  }

  amountCellIndex() {
    return 3;
  }
}

class DebitControl {
  name() {
    return "debit";
  }

  amountCellIndex() {
    return 2;
  }
}

function removeTransaction(accountTables, message, amountCellIndex) {
  const table = accountTables.get(message.name);
  for (let i = 0; i < table.rows.length; i += 1) {
    if (
      table.rows[i].cells[1].textContent === message.description &&
      table.rows[i].cells[amountCellIndex].textContent === message.amount &&
      table.rows[i].cells[4].textContent === message.date
    ) {
      table.deleteRow(i);
      return;
    }
  }
}

function main() {
  const accountsWithSummaries = createChild(document.body, "div");
  accountsWithSummaries.style.display = "flex";
  const accountSummaryTable = createChild(accountsWithSummaries, "table");
  const accountsAndControls = createChild(accountsWithSummaries, "div");
  const accounts = createChild(accountsAndControls, "div");
  const totalBalance = createChild(document.body, "div");
  const controls = createChild(accountsAndControls, "div");
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
  const transferButton = createChild(controls, "button");
  transferButton.textContent = "transfer";
  initializeAccountTable(accountSummaryTable);
  let selectedAccountTable = null;
  let selectedTransactionRow = null;
  const accountTables = new Map();
  const accountSummaryRows = new Map();
  const accountNames = new Map();
  const websocket = new WebSocket("ws://localhost:9012");
  websocket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    switch (message.method) {
      case "add account": {
        const accountSummaryRow = createChild(accountSummaryTable, "tr");
        const selection = createChild(
          createChild(accountSummaryRow, "td"),
          "input"
        );
        selection.name = "account selection";
        selection.type = "radio";
        const name = createChild(accountSummaryRow, "td");
        name.textContent = message.name;
        createChild(accountSummaryRow, "td");
        accountSummaryRows.set(message.name, accountSummaryRow);

        const accountTable = createChild(accounts, "table");
        const header = createChild(accountTable, "tr");
        createChild(header, "th");
        createChild(header, "th").textContent = "Description";
        createChild(header, "th").textContent = "Debits";
        createChild(header, "th").textContent = "Credits";
        createChild(header, "th").textContent = "Date";
        accountTables.set(message.name, accountTable);
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
        const table = accountTables.get(message.name);
        table.parentNode.removeChild(table);
        accountTables.delete(message.name);
        break;
      }
      case "update balance": {
        totalBalance.textContent = message.amount;
        break;
      }
      case "add credit": {
        const row = createChild(accountTables.get(message.name), "tr");
        const selection = createChild(createChild(row, "td"), "input");
        selection.name = "transaction selection";
        selection.type = "radio";
        createChild(row, "td").textContent = message.description;
        createChild(row, "td");
        createChild(row, "td").textContent = message.amount;
        createChild(row, "td").textContent = message.date;
        selection.addEventListener("change", () => {
          selectedTransactionRow = row;
        });
        break;
      }
      case "remove credit": {
        removeTransaction(accountTables, message, 3);
        break;
      }
      case "add debit": {
        const row = createChild(accountTables.get(message.name), "tr");
        const radioInput = createChild(createChild(row, "td"), "input");
        radioInput.name = "transaction selection";
        radioInput.type = "radio";
        createChild(row, "td").textContent = message.description;
        createChild(row, "td").textContent = message.amount;
        createChild(row, "td");
        createChild(row, "td").textContent = message.date;
        radioInput.addEventListener("change", () => {
          selectedTransactionRow = row;
        });
        break;
      }
      case "remove debit": {
        removeTransaction(accountTables, message, 2);
        break;
      }
      case "update account balance": {
        const accountSummary = accountSummaryRows.get(message.name);
        accountSummary.lastElementChild.textContent = message.amount;
        break;
      }
      default:
        break;
    }
  };
  addTransactionButton.addEventListener("click", () => {
    const control = transactionRowIsCredit(selectedTransactionRow)
      ? new CreditControl()
      : new DebitControl();
    websocket.send(
      JSON.stringify({
        method: control.name(),
        name: accountNames.get(selectedAccountTable),
        description: descriptionInput.value,
        amount: amountInput.value,
        date: dateInput.value,
      })
    );
  });
  transferButton.addEventListener("click", () => {
    if (!accountTableIsMaster(selectedAccountTable, accountNames)) {
      websocket.send(
        JSON.stringify({
          method: "transfer",
          name: accountNames.get(selectedAccountTable),
          amount: amountInput.value,
          date: dateInput.value,
        })
      );
    }
  });
  removeTransactionButton.addEventListener("click", () => {
    if (
      accountTableIsMaster(selectedAccountTable, accountNames) ===
      transactionRowIsCredit(selectedTransactionRow)
    ) {
      const control = transactionRowIsCredit(selectedTransactionRow)
        ? new CreditControl()
        : new DebitControl();
      websocket.send(
        JSON.stringify({
          method: `remove ${control.name()}`,
          name: accountNames.get(selectedAccountTable),
          description: selectedTransactionRow.cells[1].textContent,
          amount:
            selectedTransactionRow.cells[control.amountCellIndex()].textContent,
          date: selectedTransactionRow.cells[4].textContent,
        })
      );
    } else if (!accountTableIsMaster(selectedAccountTable, accountNames)) {
      websocket.send(
        JSON.stringify({
          method: "remove transfer",
          name: accountNames.get(selectedAccountTable),
          amount: selectedTransactionRow.cells[3].textContent,
          date: selectedTransactionRow.cells[4].textContent,
        })
      );
    }
  });
}

document.addEventListener("DOMContentLoaded", () => {
  main();
});
