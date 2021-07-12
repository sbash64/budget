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
    "div"
  );
  const accountSummaryTable = createChild(
    accountSummariesWithAccountControls,
    "table"
  );
  accountSummaryTable.style.border = "2px solid";
  accountSummaryTable.style.margin = "5px";

  const accountControls = createChild(
    accountSummariesWithAccountControls,
    "div"
  );
  accountControls.style.display = "flex";
  accountControls.style.flexDirection = "column";
  accountControls.style.alignItems = "flex-start";

  const removeAccountButton = createChild(accountControls, "button");
  removeAccountButton.textContent = "remove";

  const createAccountControls = createChild(accountControls, "div");
  createAccountControls.style.border = "1px solid";
  createAccountControls.style.margin = "5px";
  createAccountControls.style.padding = "10px";
  const newAccountNameLabel = createChild(createAccountControls, "label");
  newAccountNameLabel.textContent = "name";
  const newAccountNameInput = createChild(newAccountNameLabel, "input");
  newAccountNameInput.type = "text";
  const createAccountButton = createChild(createAccountControls, "button");
  createAccountButton.textContent = "create";

  const addTransactionControls = createChild(accountControls, "div");
  addTransactionControls.style.margin = "5px";
  addTransactionControls.style.padding = "10px";
  addTransactionControls.style.display = "flex";
  addTransactionControls.style.flexDirection = "column";
  addTransactionControls.style.alignItems = "flex-start";
  addTransactionControls.style.border = "1px solid";
  const addTransactionDescriptionLabel = createChild(
    addTransactionControls,
    "label"
  );
  addTransactionDescriptionLabel.textContent = "description";
  const addTransactionDescriptionInput = createChild(
    addTransactionDescriptionLabel,
    "input"
  );
  addTransactionDescriptionInput.type = "text";
  const addTransactionAmountLabel = createChild(
    addTransactionControls,
    "label"
  );
  addTransactionAmountLabel.textContent = "amount";
  const addTransactionAmountInput = createChild(
    addTransactionAmountLabel,
    "input"
  );
  addTransactionAmountInput.type = "number";
  addTransactionAmountInput.min = 0;
  addTransactionAmountInput.step = "any";
  const addTransactionDateLabel = createChild(addTransactionControls, "label");
  addTransactionDateLabel.textContent = "date";
  const addTransactionDateInput = createChild(addTransactionDateLabel, "input");
  addTransactionDateInput.type = "date";
  const addTransactionButton = createChild(addTransactionControls, "button");
  addTransactionButton.textContent = "add";

  const transferControls = createChild(accountControls, "div");
  transferControls.style.margin = "5px";
  transferControls.style.padding = "10px";
  transferControls.style.display = "flex";
  transferControls.style.flexDirection = "column";
  transferControls.style.alignItems = "flex-start";
  transferControls.style.border = "1px solid";
  const transferAmountLabel = createChild(transferControls, "label");
  transferAmountLabel.textContent = "amount";
  const transferAmountInput = createChild(transferAmountLabel, "input");
  transferAmountInput.type = "number";
  transferAmountInput.min = 0;
  transferAmountInput.step = "any";
  const transferDateLabel = createChild(transferControls, "label");
  transferDateLabel.textContent = "date";
  const transferDateInput = createChild(transferDateLabel, "input");
  transferDateInput.type = "date";
  const transferButton = createChild(transferControls, "button");
  transferButton.textContent = "transfer";

  const accountDetailViewAndTransactionControls = createChild(
    accountSummariesWithDetailView,
    "div"
  );
  const transactionTable = createChild(
    accountDetailViewAndTransactionControls,
    "div"
  );
  transactionTable.style.border = "2px solid";
  transactionTable.style.margin = "5px";

  const transactionControls = createChild(
    accountDetailViewAndTransactionControls,
    "div"
  );
  const removeTransactionButton = createChild(transactionControls, "button");
  removeTransactionButton.textContent = "remove";
  const verifyButton = createChild(transactionControls, "button");
  verifyButton.textContent = "verify";

  const accountSummaryTableHead = createChild(accountSummaryTable, "thead");
  const accountSummaryTableHeadRow = createChild(accountSummaryTableHead, "tr");
  createChild(accountSummaryTableHeadRow, "th");
  createChild(accountSummaryTableHeadRow, "th").textContent = "Name";
  createChild(accountSummaryTableHeadRow, "th").textContent = "Balance";
  const accountSummaryTableBody = createChild(accountSummaryTable, "tbody");

  const accountTableHead = createChild(transactionTable, "thead");
  const accountTableHeader = createChild(accountTableHead, "tr");
  createChild(accountTableHeader, "th");
  createChild(accountTableHeader, "th").textContent = "Description";
  createChild(accountTableHeader, "th").textContent = "Debits";
  createChild(accountTableHeader, "th").textContent = "Credits";
  createChild(accountTableHeader, "th").textContent = "Date";
  createChild(accountTableHeader, "th").textContent = "Verified";

  const totalBalance = createChild(document.body, "div");
  const saveButton = createChild(document.body, "button");
  saveButton.textContent = "save";

  let selectedAccountTableBody = null;
  let selectedTransactionRow = null;
  let selectedAccountSummaryRow = null;
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
        createChild(accountSummaryRow, "td").style.textAlign = "right";
        accountSummaryRows.push(accountSummaryRow);

        const accountTableBody = createChild(transactionTable, "tbody");
        accountTableBodies.push(accountTableBody);

        accountTableBody.style.display = "none";
        selection.addEventListener("change", () => {
          if (selectedAccountTableBody)
            selectedAccountTableBody.style.display = "none";
          accountTableBody.style.display = "";
          selectedAccountTableBody = accountTableBody;
          selectedAccountSummaryRow = accountSummaryRow;
        });
        break;
      }
      case "remove account": {
        const [body] = accountTableBodies.splice(message.accountIndex, 1);
        body.parentNode.removeChild(body);
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
        createChild(row, "td").style.textAlign = "right";
        createChild(row, "td").style.textAlign = "right";
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
  saveButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "save",
    });
  });
  removeAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "remove account",
      name: selectedAccountSummaryRow.cells[1].textContent,
    });
  });
  createAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "create account",
      name: newAccountNameInput.value,
      amount: addTransactionAmountInput.value,
      date: addTransactionDateInput.value,
    });
  });
  addTransactionButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "add transaction",
      name: selectedAccountSummaryRow.cells[1].textContent,
      description: addTransactionDescriptionInput.value,
      amount: addTransactionAmountInput.value,
      date: addTransactionDateInput.value,
    });
  });
  transferButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "transfer",
      name: selectedAccountSummaryRow.cells[1].textContent,
      amount: transferAmountInput.value,
      date: transferDateInput.value,
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
