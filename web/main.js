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
  const page = createChild(document.body, "div");
  page.style.display = "grid";
  const tbdItems = createChild(page, "div");
  tbdItems.style.gridRow = 1;

  const totalBalance = createChild(tbdItems, "div");
  const saveButton = createChild(tbdItems, "button");
  saveButton.textContent = "save";
  const reduceButton = createChild(tbdItems, "button");
  reduceButton.textContent = "reduce";
  const restoreButton = createChild(tbdItems, "button");
  restoreButton.textContent = "restore";

  const budgetViewAndControls = createChild(page, "div");
  budgetViewAndControls.style.display = "grid";
  budgetViewAndControls.style.gridRow = 2;

  const tableViews = createChild(budgetViewAndControls, "div");
  tableViews.style.gridRow = 2;
  tableViews.style.display = "grid";

  const leftHandTableView = createChild(tableViews, "div");
  leftHandTableView.style.gridColumn = 1;
  leftHandTableView.style.display = "grid";

  const rightHandTableView = createChild(tableViews, "div");
  rightHandTableView.style.gridColumn = 2;
  rightHandTableView.style.display = "grid";

  const leftHandTableViewButtons = createChild(leftHandTableView, "div");
  leftHandTableView.style.gridRow = 1;

  const rightHandTableViewButtons = createChild(rightHandTableView, "div");
  rightHandTableViewButtons.style.gridRow = 1;

  const accountSummaryTableWrapper = createChild(leftHandTableView, "div");
  accountSummaryTableWrapper.style.gridRow = 2;
  accountSummaryTableWrapper.style.height = "20em";
  accountSummaryTableWrapper.style.overflowY = "scroll";

  const transactionTableWrapper = createChild(rightHandTableView, "div");
  transactionTableWrapper.style.gridRow = 2;
  transactionTableWrapper.style.height = "20em";
  transactionTableWrapper.style.overflowY = "scroll";

  const removeAccountButton = createChild(leftHandTableViewButtons, "button");
  removeAccountButton.textContent = "remove";
  const closeAccountButton = createChild(leftHandTableViewButtons, "button");
  closeAccountButton.textContent = "close";

  const removeTransactionButton = createChild(
    rightHandTableViewButtons,
    "button"
  );
  removeTransactionButton.textContent = "remove";
  const verifyButton = createChild(rightHandTableViewButtons, "button");
  verifyButton.textContent = "verify";

  const accountSummaryTable = createChild(accountSummaryTableWrapper, "table");
  accountSummaryTable.style.border = "2px solid";
  accountSummaryTable.style.margin = "5px";
  accountSummaryTable.style.tableLayout = "fixed";
  accountSummaryTable.style.borderCollapse = "collapse";

  const accountFormControls = createChild(budgetViewAndControls, "div");
  accountFormControls.style.gridRow = 3;

  const createAccountControls = createChild(accountFormControls, "section");
  createChild(createAccountControls, "h4").textContent = "Create Account";
  const newAccountNameLabel = createChild(createAccountControls, "label");
  newAccountNameLabel.textContent = "name";
  const newAccountNameInput = createChild(newAccountNameLabel, "input");
  newAccountNameInput.type = "text";
  const createAccountButton = createChild(createAccountControls, "button");
  createAccountButton.textContent = "create";

  const addTransactionControls = createChild(accountFormControls, "section");
  createChild(addTransactionControls, "h4").textContent =
    "Add Transaction to Account";
  addTransactionControls.style.display = "flex";
  addTransactionControls.style.flexDirection = "column";
  addTransactionControls.style.alignItems = "flex-start";
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

  const transferControls = createChild(accountFormControls, "section");
  createChild(transferControls, "h4").textContent = "Transfer to Account";
  transferControls.style.display = "flex";
  transferControls.style.flexDirection = "column";
  transferControls.style.alignItems = "flex-start";
  const transferAmountLabel = createChild(transferControls, "label");
  transferAmountLabel.textContent = "amount";
  const transferAmountInput = createChild(transferAmountLabel, "input");
  transferAmountInput.type = "number";
  transferAmountInput.min = 0;
  transferAmountInput.step = "any";
  const transferButton = createChild(transferControls, "button");
  transferButton.textContent = "transfer";

  const allocateControls = createChild(accountFormControls, "section");
  createChild(allocateControls, "h4").textContent = "Allocate Account";
  allocateControls.style.display = "flex";
  allocateControls.style.flexDirection = "column";
  allocateControls.style.alignItems = "flex-start";
  const allocateAmountLabel = createChild(allocateControls, "label");
  allocateAmountLabel.textContent = "amount";
  const allocateAmountInput = createChild(allocateAmountLabel, "input");
  allocateAmountInput.type = "number";
  allocateAmountInput.min = 0;
  allocateAmountInput.step = "any";
  const allocateButton = createChild(allocateControls, "button");
  allocateButton.textContent = "allocate";

  const transactionTable = createChild(transactionTableWrapper, "table");
  transactionTable.style.border = "2px solid";
  transactionTable.style.margin = "5px";
  transactionTable.style.tableLayout = "fixed";
  transactionTable.style.borderCollapse = "collapse";

  const accountSummaryTableHead = createChild(accountSummaryTable, "thead");
  const accountSummaryTableHeadRow = createChild(accountSummaryTableHead, "tr");
  createChild(accountSummaryTableHeadRow, "th");
  const accountSummaryNameHeaderElement = createChild(
    accountSummaryTableHeadRow,
    "th"
  );
  accountSummaryNameHeaderElement.textContent = "Name";
  accountSummaryNameHeaderElement.style.width = "20ch";
  const accountSummaryFundsHeaderElement = createChild(
    accountSummaryTableHeadRow,
    "th"
  );
  accountSummaryFundsHeaderElement.textContent = "Funds";
  accountSummaryFundsHeaderElement.style.width = "9ch";
  const accountSummaryBalanceHeaderElement = createChild(
    accountSummaryTableHeadRow,
    "th"
  );
  accountSummaryBalanceHeaderElement.textContent = "Balance";
  accountSummaryBalanceHeaderElement.style.width = "9ch";
  const accountSummaryTableBody = createChild(accountSummaryTable, "tbody");

  const accountTableHead = createChild(transactionTable, "thead");
  const accountTableHeader = createChild(accountTableHead, "tr");
  createChild(accountTableHeader, "th");
  const descriptionHeaderElement = createChild(accountTableHeader, "th");
  descriptionHeaderElement.textContent = "Description";
  descriptionHeaderElement.style.width = "30ch";
  const debitHeaderElement = createChild(accountTableHeader, "th");
  debitHeaderElement.textContent = "Debits";
  debitHeaderElement.style.width = "9ch";
  const creditHeaderElement = createChild(accountTableHeader, "th");
  creditHeaderElement.textContent = "Credits";
  creditHeaderElement.style.width = "9ch";
  const dateHeaderElement = createChild(accountTableHeader, "th");
  dateHeaderElement.textContent = "Date";
  dateHeaderElement.style.width = "12ch";
  createChild(accountTableHeader, "th").textContent = "Verified";

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
        createChild(row, "td").style.textAlign = "center";
        createChild(row, "td").style.textAlign = "center";
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
      case "update account funds": {
        accountSummaryRows[message.accountIndex].cells[2].textContent =
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
  reduceButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "reduce",
    });
  });
  restoreButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "restore",
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
  closeAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "close account",
      name: selectedAccountSummaryRow.cells[1].textContent,
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
    });
  });
  allocateButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "allocate",
      name: selectedAccountSummaryRow.cells[1].textContent,
      amount: allocateAmountInput.value,
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
