function createChild(parent, tagName) {
  const child = document.createElement(tagName);
  parent.append(child);
  return child;
}

function sendMessage(websocket, message) {
  websocket.send(JSON.stringify(message));
}

function transactionMessage(
  selectedAccountSummaryRow,
  selectedTransactionRow,
  method
) {
  return {
    method,
    name: selectedAccountSummaryRow.cells[1].textContent,
    description: selectedTransactionRow.cells[1].textContent,
    debitAmount: selectedTransactionRow.cells[2].textContent,
    creditAmount: selectedTransactionRow.cells[3].textContent,
    date: selectedTransactionRow.cells[4].textContent,
  };
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
  const newAccountNameLabel = createChild(accountControls, "label");
  newAccountNameLabel.textContent = "name";
  const nameInput = createChild(newAccountNameLabel, "input");
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
  const addTransactionControls = createChild(transactionControls, "div");
  addTransactionControls.style.display = "flex";
  addTransactionControls.style.flexDirection = "column";
  const descriptionLabel = createChild(addTransactionControls, "label");
  descriptionLabel.textContent = "description";
  const descriptionInput = createChild(descriptionLabel, "input");
  descriptionInput.type = "text";
  const amountLabel = createChild(addTransactionControls, "label");
  amountLabel.textContent = "amount";
  const amountInput = createChild(amountLabel, "input");
  amountInput.type = "number";
  amountInput.min = 0;
  amountInput.step = "any";
  const dateLabel = createChild(addTransactionControls, "label");
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
  const verifyButton = createChild(transactionButtons, "button");
  verifyButton.textContent = "verify";
  const accountSummaryTableHead = createChild(accountSummaryTable, "thead");
  const accountSummaryTableHeadRow = createChild(accountSummaryTableHead, "tr");
  createChild(accountSummaryTableHeadRow, "th");
  createChild(accountSummaryTableHeadRow, "th").textContent = "Name";
  createChild(accountSummaryTableHeadRow, "th").textContent = "Balance";
  const accountSummaryTableBody = createChild(accountSummaryTable, "tbody");
  let selectedAccountTable = null;
  let selectedTransactionRow = null;
  let selectedAccountSummaryRow = null;
  const accountTables = [];
  const accountTableBodies = [];
  const accountSummaryRows = [];
  const websocket = new WebSocket("ws://localhost:9012");
  websocket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    switch (message.method) {
      case "add account": {
        const accountSummaryRow = createChild(accountSummaryTableBody, "tr");
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
        const accountTableHead = createChild(accountTable, "thead");
        const header = createChild(accountTableHead, "tr");
        createChild(header, "th");
        createChild(header, "th").textContent = "Description";
        createChild(header, "th").textContent = "Debits";
        createChild(header, "th").textContent = "Credits";
        createChild(header, "th").textContent = "Date";
        createChild(header, "th").textContent = "Verified";
        const accountTableBody = createChild(accountTable, "tbody");
        accountTables.push(accountTable);
        accountTableBodies.push(accountTableBody);

        accountTable.style.display = "none";
        selection.addEventListener("change", () => {
          if (selectedAccountTable) selectedAccountTable.style.display = "none";
          accountTable.style.display = "block";
          selectedAccountTable = accountTable;
          selectedAccountSummaryRow = accountSummaryRow;
        });
        break;
      }
      case "remove account": {
        const [table] = accountTables.splice(message.accountIndex, 1);
        table.parentNode.removeChild(table);
        const [row] = accountSummaryRows.splice(message.accountIndex, 1);
        row.parentNode.removeChild(row);
        break;
      }
      case "update total balance": {
        totalBalance.textContent = message.amount;
        break;
      }
      case "add transaction": {
        const row = createChild(accountTableBodies[message.accountIndex], "tr");
        const selection = createChild(createChild(row, "td"), "input");
        selection.name = "transaction selection";
        selection.type = "radio";
        createChild(row, "td");
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
        accountTableBodies[message.accountIndex].deleteRow(
          message.transactionIndex
        );
        break;
      }
      case "update transaction": {
        accountTableBodies[message.accountIndex].rows[
          message.transactionIndex
        ].cells[1].textContent = message.description;
        accountTableBodies[message.accountIndex].rows[
          message.transactionIndex
        ].cells[2].textContent = message.debitAmount;
        accountTableBodies[message.accountIndex].rows[
          message.transactionIndex
        ].cells[3].textContent = message.creditAmount;
        accountTableBodies[message.accountIndex].rows[
          message.transactionIndex
        ].cells[4].textContent = message.date;
        break;
      }
      case "verify transaction": {
        accountTableBodies[message.accountIndex].rows[
          message.transactionIndex
        ].cells[5].textContent = "✅";
        break;
      }
      default:
        break;
    }
  };
  removeAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "remove account",
      name: selectedAccountSummaryRow.cells[1].textContent,
    });
  });
  createAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "create account",
      name: nameInput.value,
      amount: amountInput.value,
      date: dateInput.value,
    });
  });
  addTransactionButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "add transaction",
      name: selectedAccountSummaryRow.cells[1].textContent,
      description: descriptionInput.value,
      amount: amountInput.value,
      date: dateInput.value,
    });
  });
  transferButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "transfer",
      name: selectedAccountSummaryRow.cells[1].textContent,
      amount: amountInput.value,
      date: dateInput.value,
    });
  });
  verifyButton.addEventListener("click", () => {
    sendMessage(
      websocket,
      transactionMessage(
        selectedAccountSummaryRow,
        selectedTransactionRow,
        "verify transaction"
      )
    );
  });
  removeTransactionButton.addEventListener("click", () => {
    sendMessage(
      websocket,
      transactionMessage(
        selectedAccountSummaryRow,
        selectedTransactionRow,
        "remove transaction"
      )
    );
  });
}

document.addEventListener("DOMContentLoaded", () => {
  main();
});
