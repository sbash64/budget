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
  return accountNames.get(selectedAccountTable) === "master";
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

function main() {
  const accountSummariesWithDetailView = createChild(document.body, "div");
  accountSummariesWithDetailView.style.display = "flex";
  const accountSummariesWithAccountControls = createChild(
    accountSummariesWithDetailView,
    "table"
  );
  const accountSummaryTable = createChild(
    accountSummariesWithAccountControls,
    "table"
  );
  const accountsAndTransactionControls = createChild(
    accountSummariesWithDetailView,
    "div"
  );
  const accountControls = createChild(
    accountSummariesWithAccountControls,
    "div"
  );
  const nameLabel = createChild(accountControls, "label");
  nameLabel.textContent = "name";
  const nameInput = createChild(nameLabel, "input");
  nameInput.type = "text";
  const accountButtons = createChild(accountControls, "div");
  const createAccountButton = createChild(accountButtons, "button");
  createAccountButton.textContent = "create";
  const removeAccountButton = createChild(accountButtons, "button");
  removeAccountButton.textContent = "remove";
  const accounts = createChild(accountsAndTransactionControls, "div");
  const totalBalance = createChild(document.body, "div");
  const transactionControls = createChild(
    accountsAndTransactionControls,
    "div"
  );
  const descriptionLabel = createChild(transactionControls, "label");
  descriptionLabel.textContent = "description";
  const descriptionInput = createChild(descriptionLabel, "input");
  descriptionInput.type = "text";
  const amountLabel = createChild(transactionControls, "label");
  amountLabel.textContent = "amount";
  const amountInput = createChild(amountLabel, "input");
  amountInput.type = "number";
  amountInput.min = 0;
  amountInput.step = "any";
  const dateLabel = createChild(transactionControls, "label");
  dateLabel.textContent = "date";
  const dateInput = createChild(dateLabel, "input");
  dateInput.type = "date";
  const transactionButtons = createChild(transactionControls, "div");
  const addTransactionButton = createChild(transactionButtons, "button");
  addTransactionButton.textContent = "add";
  const removeTransactionButton = createChild(transactionButtons, "button");
  removeTransactionButton.textContent = "remove";
  const transferButton = createChild(transactionButtons, "button");
  transferButton.textContent = "transfer";
  initializeAccountTable(accountSummaryTable);
  let selectedAccountTable = null;
  let selectedTransactionRow = null;
  const accountTables = [];
  const accountSummaryRows = [];
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
        accountSummaryRows.push(accountSummaryRow);

        const accountTable = createChild(accounts, "table");
        const header = createChild(accountTable, "tr");
        createChild(header, "th");
        createChild(header, "th").textContent = "Description";
        createChild(header, "th").textContent = "Debits";
        createChild(header, "th").textContent = "Credits";
        createChild(header, "th").textContent = "Date";
        accountTables.push(accountTable);
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
        const table = accountTables.splice(message.accountIndex, 1);
        table.parentNode.removeChild(table);
        const row = accountSummaryRows.splice(message.accountIndex, 1);
        row.parentNode.removeChild(row);
        break;
      }
      case "update total balance": {
        totalBalance.textContent = message.amount;
        break;
      }
      case "add credit": {
        const row = createChild(accountTables[message.accountIndex], "tr");
        const selection = createChild(createChild(row, "td"), "input");
        selection.name = "transaction selection";
        selection.type = "radio";
        createChild(row, "td");
        createChild(row, "td");
        createChild(row, "td");
        createChild(row, "td");
        selection.addEventListener("change", () => {
          selectedTransactionRow = row;
        });
        break;
      }
      case "add debit": {
        const row = createChild(accountTables[message.accountIndex], "tr");
        const selection = createChild(createChild(row, "td"), "input");
        selection.name = "transaction selection";
        selection.type = "radio";
        createChild(row, "td");
        createChild(row, "td");
        createChild(row, "td");
        createChild(row, "td");
        selection.addEventListener("change", () => {
          selectedTransactionRow = row;
        });
        break;
      }
      case "update account balance": {
        accountSummaryRows[message.accountIndex].lastElementChild.textContent =
          message.amount;
        break;
      }
      case "remove transaction": {
        accountTables[message.accountIndex].deleteRow(message.transactionIndex);
        break;
      }
      case "update transaction": {
        accountTables[message.accountIndex].rows[
          message.transactionIndex
        ].cells[1].textContent = message.description;
        accountTables[message.accountIndex].rows[
          message.transactionIndex
        ].cells[2].textContent = message.amount;
        accountTables[message.accountIndex].rows[
          message.transactionIndex
        ].cells[4].textContent = message.date;
        break;
      }
      case "verify transaction":
        break;
      default:
        break;
    }
  };
  removeAccountButton.addEventListener("click", () => {
    websocket.send(
      JSON.stringify({
        method: "remove account",
        name: accountNames.get(selectedAccountTable),
      })
    );
  });
  createAccountButton.addEventListener("click", () => {
    websocket.send(
      JSON.stringify({
        method: "create account",
        name: nameInput.value,
        amount: amountInput.value,
        date: dateInput.value,
      })
    );
  });
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
