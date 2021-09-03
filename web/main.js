function createChild(parent, tagName) {
  const child = document.createElement(tagName);
  parent.append(child);
  return child;
}

function sendMessage(websocket, message) {
  websocket.send(JSON.stringify(message));
}

function accountName(selectedAccountSummaryRow) {
  return selectedAccountSummaryRow.cells[1].textContent;
}

function transactionMessage(
  selectedAccountSummaryRow,
  selectedTransactionRow,
  method
) {
  return {
    method,
    name: accountName(selectedAccountSummaryRow),
    description: selectedTransactionRow.cells[1].textContent,
    amount: selectedTransactionRow.cells[2].textContent,
    date: selectedTransactionRow.cells[3].textContent,
  };
}

function updateTransaction(row, message) {
  row.cells[1].textContent = message.description;
  row.cells[2].textContent = message.amount;
  row.cells[3].textContent = message.date;
}

function accountTableBody(accountTableBodies, message) {
  return accountTableBodies[message.accountIndex];
}

function transactionRow(accountTableBodies, message) {
  return accountTableBody(accountTableBodies, message).rows[
    message.transactionIndex
  ];
}

function accountSummaryRow(accountSummaryRows, message) {
  return accountSummaryRows[message.accountIndex];
}

function main() {
  const page = createChild(document.body, "div");
  page.style.display = "grid";
  const topPage = createChild(page, "div");
  topPage.style.gridRow = 1;
  topPage.style.display = "grid";

  const netIncomeLabel = createChild(topPage, "label");
  netIncomeLabel.textContent = "Net Income";
  netIncomeLabel.gridRow = 1;

  const netIncome = createChild(netIncomeLabel, "strong");
  netIncome.style.margin = "1ch";

  const topPageButtons = createChild(topPage, "div");
  topPageButtons.gridRow = 2;

  const saveButton = createChild(topPageButtons, "button");
  saveButton.textContent = "save";
  const reduceButton = createChild(topPageButtons, "button");
  reduceButton.textContent = "reduce";
  const restoreButton = createChild(topPageButtons, "button");
  restoreButton.textContent = "restore";

  const pageBody = createChild(page, "div");
  pageBody.style.display = "grid";
  pageBody.style.gridRow = 2;

  const tableViews = createChild(pageBody, "div");
  tableViews.style.gridRow = 2;
  tableViews.style.display = "flex";
  tableViews.style.flexDirection = "row";

  const leftHandContent = createChild(tableViews, "div");
  const leftHandContentHeader = createChild(leftHandContent, "h3");
  leftHandContentHeader.textContent = "Account Summaries";

  const rightHandContent = createChild(tableViews, "div");
  const rightHandContentHeader = createChild(rightHandContent, "h3");

  const leftHandTableView = createChild(leftHandContent, "div");
  leftHandTableView.style.display = "grid";
  leftHandTableView.style.justifyItems = "end";

  const rightHandTableView = createChild(rightHandContent, "div");
  rightHandTableView.style.display = "grid";
  rightHandTableView.style.justifyItems = "end";

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
  const verifyTransactionButton = createChild(
    rightHandTableViewButtons,
    "button"
  );
  verifyTransactionButton.textContent = "verify";

  const accountSummaryTable = createChild(accountSummaryTableWrapper, "table");
  accountSummaryTable.style.border = "2px solid";
  accountSummaryTable.style.margin = "5px";
  accountSummaryTable.style.tableLayout = "fixed";
  accountSummaryTable.style.borderCollapse = "collapse";

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
  const accountSummaryAllocationHeaderElement = createChild(
    accountSummaryTableHeadRow,
    "th"
  );
  accountSummaryAllocationHeaderElement.textContent = "Allocation";
  accountSummaryAllocationHeaderElement.style.width = "9ch";
  const accountSummaryBalanceHeaderElement = createChild(
    accountSummaryTableHeadRow,
    "th"
  );
  accountSummaryBalanceHeaderElement.textContent = "Balance";
  accountSummaryBalanceHeaderElement.style.width = "9ch";
  const accountSummaryTableBody = createChild(accountSummaryTable, "tbody");

  const transactionTableHead = createChild(transactionTable, "thead");
  const transactionTableHeader = createChild(transactionTableHead, "tr");
  createChild(transactionTableHeader, "th");
  const transactionDescriptionHeaderElement = createChild(
    transactionTableHeader,
    "th"
  );
  transactionDescriptionHeaderElement.textContent = "Description";
  transactionDescriptionHeaderElement.style.width = "30ch";
  const transactionAmountHeaderElement = createChild(
    transactionTableHeader,
    "th"
  );
  transactionAmountHeaderElement.textContent = "Amount";
  transactionAmountHeaderElement.style.width = "9ch";
  const transactionDateHeaderElement = createChild(
    transactionTableHeader,
    "th"
  );
  transactionDateHeaderElement.textContent = "Date";
  transactionDateHeaderElement.style.width = "12ch";
  createChild(transactionTableHeader, "th").textContent = "Verified";

  const formControls = createChild(pageBody, "div");
  formControls.style.gridRow = 3;
  formControls.style.display = "grid";

  const leftHandFormControls = createChild(formControls, "div");
  leftHandFormControls.style.gridColumn = 1;
  const rightHandFormControls = createChild(formControls, "div");
  rightHandFormControls.style.gridColumn = 2;

  const createAccountControls = createChild(leftHandFormControls, "section");
  createChild(createAccountControls, "h4").textContent = "Create Account";
  const newAccountNameLabel = createChild(createAccountControls, "label");
  newAccountNameLabel.textContent = "name";
  const newAccountNameInput = createChild(newAccountNameLabel, "input");
  newAccountNameInput.type = "text";
  newAccountNameInput.style.margin = "1ch";
  const createAccountButton = createChild(createAccountControls, "button");
  createAccountButton.textContent = "create";

  const addTransactionControls = createChild(rightHandFormControls, "section");
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
  addTransactionDescriptionInput.style.margin = "1ch";
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
  addTransactionAmountInput.style.margin = "1ch";
  const addTransactionDateLabel = createChild(addTransactionControls, "label");
  addTransactionDateLabel.textContent = "date";
  const addTransactionDateInput = createChild(addTransactionDateLabel, "input");
  addTransactionDateInput.type = "date";
  addTransactionDateInput.style.margin = "1ch";
  const addTransactionButton = createChild(addTransactionControls, "button");
  addTransactionButton.textContent = "add";

  const transferControls = createChild(leftHandFormControls, "section");
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
  transferAmountInput.style.margin = "1ch";
  const transferButton = createChild(transferControls, "button");
  transferButton.textContent = "transfer";

  const allocateControls = createChild(leftHandFormControls, "section");
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
  allocateAmountInput.style.margin = "1ch";
  const allocateButton = createChild(allocateControls, "button");
  allocateButton.textContent = "allocate";

  let selectedAccountTransactionTableBody = null;
  let selectedTransactionRow = null;
  let selectedAccountSummaryRow = null;
  const accountTableBodies = [];
  const accountSummaryRows = [];
  const websocket = new WebSocket(`ws://${window.location.host}`);
  websocket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    switch (message.method) {
      case "update net income": {
        netIncome.textContent = message.amount;
        break;
      }
      case "add account": {
        const row = createChild(accountSummaryTableBody, "tr");
        const selection = createChild(createChild(row, "td"), "input");
        selection.name = "account selection";
        selection.type = "radio";
        const name = createChild(row, "td");
        name.textContent = message.name;
        createChild(row, "td").style.textAlign = "right";
        createChild(row, "td").style.textAlign = "right";
        accountSummaryRows.push(row);

        const transactionTableBody = createChild(transactionTable, "tbody");
        accountTableBodies.push(transactionTableBody);

        if (selectedAccountTransactionTableBody)
          selectedAccountTransactionTableBody.style.display = "none";
        rightHandContentHeader.textContent = message.name;
        selectedAccountTransactionTableBody = transactionTableBody;
        selectedAccountSummaryRow = row;
        selection.checked = true;

        selection.addEventListener("change", () => {
          if (selectedAccountTransactionTableBody)
            selectedAccountTransactionTableBody.style.display = "none";
          transactionTableBody.style.display = "";
          rightHandContentHeader.textContent = accountName(row);
          selectedAccountTransactionTableBody = transactionTableBody;
          selectedAccountSummaryRow = row;
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
      case "add transaction": {
        const row = createChild(
          accountTableBody(accountTableBodies, message),
          "tr"
        );
        const selection = createChild(createChild(row, "td"), "input");
        selection.name = "transaction selection";
        selection.type = "radio";
        createChild(row, "td");
        createChild(row, "td").style.textAlign = "right";
        createChild(row, "td").style.textAlign = "center";
        createChild(row, "td").style.textAlign = "center";
        selection.addEventListener("change", () => {
          selectedTransactionRow = row;
        });
        break;
      }
      case "remove transaction":
        accountTableBody(accountTableBodies, message).deleteRow(
          message.transactionIndex
        );
        break;
      case "update account balance":
        accountSummaryRow(
          accountSummaryRows,
          message
        ).lastElementChild.textContent = message.amount;
        break;
      case "update account allocation":
      case "update unallocated income":
        accountSummaryRow(accountSummaryRows, message).cells[2].textContent =
          message.amount;
        break;
      case "update transaction":
        updateTransaction(transactionRow(accountTableBodies, message), message);
        break;
      case "verify transaction":
        transactionRow(accountTableBodies, message).cells[4].textContent = "✅";
        break;
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
  createAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "create account",
      name: newAccountNameInput.value,
    });
    newAccountNameInput.value = "";
  });
  removeAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "remove account",
      name: accountName(selectedAccountSummaryRow),
    });
  });
  closeAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "close account",
      name: accountName(selectedAccountSummaryRow),
    });
  });
  transferButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "transfer",
      name: accountName(selectedAccountSummaryRow),
      amount: transferAmountInput.value,
    });
    transferAmountInput.value = "";
  });
  allocateButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "allocate",
      name: accountName(selectedAccountSummaryRow),
      amount: allocateAmountInput.value,
    });
    allocateAmountInput.value = "";
  });
  addTransactionButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "add transaction",
      name: accountName(selectedAccountSummaryRow),
      description: addTransactionDescriptionInput.value,
      amount: addTransactionAmountInput.value,
      date: addTransactionDateInput.value,
    });
    addTransactionDescriptionInput.value = "";
    addTransactionAmountInput.value = "";
    addTransactionDateInput.value = "";
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
  verifyTransactionButton.addEventListener("click", () => {
    sendMessage(
      websocket,
      transactionMessage(
        selectedAccountSummaryRow,
        selectedTransactionRow,
        "verify transaction"
      )
    );
  });
}

document.addEventListener("DOMContentLoaded", () => {
  main();
});
