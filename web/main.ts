import "./styles.css";

function adoptChild(parent: HTMLElement, child: HTMLElement) {
  parent.append(child);
}

function createChild(parent: HTMLElement, tagName: string): HTMLElement {
  const child = document.createElement(tagName);
  parent.append(child);
  return child;
}

interface OutgoingMessage {
  method: string;
  name?: string;
  amount?: string;
  description?: string;
  date?: string;
}

function sendMessage(websocket: WebSocket, message: OutgoingMessage) {
  websocket.send(JSON.stringify(message));
}

function accountName(selectedAccountSummaryRow: HTMLTableRowElement): string {
  return selectedAccountSummaryRow.cells[1].textContent || "";
}

function transactionMessage(
  selectedAccountSummaryRow: HTMLTableRowElement,
  selectedTransactionRow: HTMLTableRowElement,
  method: string
) {
  return {
    method,
    name: accountName(selectedAccountSummaryRow),
    description: selectedTransactionRow.cells[1].textContent || "",
    amount: selectedTransactionRow.cells[2].textContent || "",
    date: selectedTransactionRow.cells[3].textContent || "",
  };
}

interface IncomingMessage {
  description: string;
  amount: string;
  date: string;
  accountIndex: number;
  transactionIndex: number;
}

function updateTransaction(row: HTMLTableRowElement, message: IncomingMessage) {
  row.cells[1].textContent = message.description;
  row.cells[2].textContent = message.amount;
  row.cells[3].textContent = message.date;
}

function accountTableBody(
  accountTableBodies: HTMLTableSectionElement[],
  message: IncomingMessage
): HTMLTableSectionElement {
  return accountTableBodies[message.accountIndex];
}

function transactionRow(
  accountTableBodies: HTMLTableSectionElement[],
  message: IncomingMessage
) {
  return accountTableBody(accountTableBodies, message).rows[
    message.transactionIndex
  ];
}

function accountSummaryRow(
  accountSummaryTableBody: HTMLTableSectionElement,
  message: IncomingMessage
): HTMLTableRowElement {
  return accountSummaryTableBody.rows[message.accountIndex];
}

function sendOnClick(
  button: HTMLElement,
  websocket: WebSocket,
  messageFunctor: () => OutgoingMessage,
  sendFilter: () => boolean = function () {
    return true;
  }
) {
  button.addEventListener("click", () => {
    if (sendFilter()) {
      sendMessage(websocket, messageFunctor());
    }
  });
}

function main() {
  const netIncomeLabel = document.createElement("label");
  adoptChild(document.body, netIncomeLabel);
  netIncomeLabel.textContent = "Net Income";
  netIncomeLabel.style.fontSize = "24px";

  const netIncome = document.createElement("strong");
  adoptChild(netIncomeLabel, netIncome);
  netIncome.style.margin = "1ch";

  const topPageButtons = document.createElement("div");
  adoptChild(document.body, topPageButtons);

  const saveButton = document.createElement("button");
  adoptChild(topPageButtons, saveButton);
  saveButton.textContent = "save";
  const reduceButton = document.createElement("button");
  adoptChild(topPageButtons, reduceButton);
  reduceButton.textContent = "reduce";
  const restoreButton = document.createElement("button");
  adoptChild(topPageButtons, restoreButton);
  restoreButton.textContent = "restore";

  const tableViews = document.createElement("div");
  adoptChild(document.body, tableViews);
  tableViews.style.display = "grid";
  tableViews.style.gridTemplateColumns = "1fr 2fr";
  tableViews.style.columnGap = "20px";

  const leftHandContent = document.createElement("div");
  adoptChild(tableViews, leftHandContent);
  leftHandContent.style.gridColumn = "1";
  const rightHandContent = document.createElement("div");
  adoptChild(tableViews, rightHandContent);
  rightHandContent.style.gridColumn = "2";

  const aboveLeftHandTable = document.createElement("div");
  adoptChild(leftHandContent, aboveLeftHandTable);
  aboveLeftHandTable.style.display = "flex";

  const leftHandTableTitle = document.createElement("div");
  adoptChild(aboveLeftHandTable, leftHandTableTitle);
  leftHandTableTitle.textContent = "Account Summaries";
  leftHandTableTitle.style.fontSize = "28px";
  leftHandTableTitle.style.fontWeight = "bold";
  leftHandTableTitle.style.marginRight = "auto";

  const leftHandTableViewButtons = document.createElement("div");
  adoptChild(aboveLeftHandTable, leftHandTableViewButtons);

  const aboveRightHandTable = document.createElement("div");
  adoptChild(rightHandContent, aboveRightHandTable);
  aboveRightHandTable.style.display = "flex";

  const rightHandTableTitle = document.createElement("div");
  adoptChild(aboveRightHandTable, rightHandTableTitle);
  rightHandTableTitle.style.fontSize = "28px";
  rightHandTableTitle.style.fontWeight = "bold";
  rightHandTableTitle.style.marginRight = "auto";

  const rightHandTableViewButtons = document.createElement("div");
  adoptChild(aboveRightHandTable, rightHandTableViewButtons);

  const removeAccountButton = document.createElement("button");
  adoptChild(leftHandTableViewButtons, removeAccountButton);
  removeAccountButton.textContent = "remove";
  const closeAccountButton = document.createElement("button");
  adoptChild(leftHandTableViewButtons, closeAccountButton);
  closeAccountButton.textContent = "close";

  const removeTransactionButton = document.createElement("button");
  adoptChild(rightHandTableViewButtons, removeTransactionButton);
  removeTransactionButton.textContent = "remove";
  const verifyTransactionButton = document.createElement("button");
  adoptChild(rightHandTableViewButtons, verifyTransactionButton);
  verifyTransactionButton.textContent = "verify";

  const accountSummaryTable = document.createElement("table");
  adoptChild(leftHandContent, accountSummaryTable);
  accountSummaryTable.style.display = "block";
  accountSummaryTable.style.height = "24em";
  accountSummaryTable.style.overflowY = "scroll";
  accountSummaryTable.style.border = "2px solid";
  accountSummaryTable.style.margin = "5px";
  accountSummaryTable.style.tableLayout = "fixed";
  accountSummaryTable.style.borderCollapse = "collapse";

  const transactionTable = document.createElement("table");
  adoptChild(rightHandContent, transactionTable);
  transactionTable.style.display = "block";
  transactionTable.style.height = "24em";
  transactionTable.style.overflowY = "scroll";
  transactionTable.style.border = "2px solid";
  transactionTable.style.margin = "5px";
  transactionTable.style.tableLayout = "fixed";
  transactionTable.style.borderCollapse = "collapse";

  const accountSummaryTableHead = document.createElement("thead");
  adoptChild(accountSummaryTable, accountSummaryTableHead);
  const accountSummaryTableHeadRow = document.createElement("tr");
  adoptChild(accountSummaryTableHead, accountSummaryTableHeadRow);
  createChild(accountSummaryTableHeadRow, "th");
  const accountSummaryNameHeaderElement = document.createElement("th");
  adoptChild(accountSummaryTableHeadRow, accountSummaryNameHeaderElement);
  accountSummaryNameHeaderElement.textContent = "Name";
  accountSummaryNameHeaderElement.style.width = "20ch";
  const accountSummaryAllocationHeaderElement = document.createElement("th");
  adoptChild(accountSummaryTableHeadRow, accountSummaryAllocationHeaderElement);
  accountSummaryAllocationHeaderElement.textContent = "Allocation";
  accountSummaryAllocationHeaderElement.style.width = "9ch";
  const accountSummaryBalanceHeaderElement = document.createElement("th");
  adoptChild(accountSummaryTableHeadRow, accountSummaryBalanceHeaderElement);
  accountSummaryBalanceHeaderElement.textContent = "Balance";
  accountSummaryBalanceHeaderElement.style.width = "9ch";
  const accountSummaryTableBody = document.createElement("tbody");
  adoptChild(accountSummaryTable, accountSummaryTableBody);

  const transactionTableHead = document.createElement("thead");
  adoptChild(transactionTable, transactionTableHead);
  const transactionTableHeader = document.createElement("tr");
  adoptChild(transactionTableHead, transactionTableHeader);
  createChild(transactionTableHeader, "th");
  const transactionDescriptionHeaderElement = document.createElement("th");
  adoptChild(transactionTableHeader, transactionDescriptionHeaderElement);
  transactionDescriptionHeaderElement.textContent = "Description";
  transactionDescriptionHeaderElement.style.width = "40ch";
  const transactionAmountHeaderElement = document.createElement("th");
  adoptChild(transactionTableHeader, transactionAmountHeaderElement);
  transactionAmountHeaderElement.textContent = "Amount";
  transactionAmountHeaderElement.style.width = "9ch";
  const transactionDateHeaderElement = document.createElement("th");
  adoptChild(transactionTableHeader, transactionDateHeaderElement);
  transactionDateHeaderElement.textContent = "Date";
  transactionDateHeaderElement.style.width = "12ch";
  createChild(transactionTableHeader, "th").textContent = "Verified";

  const leftHandFormControls = document.createElement("div");
  adoptChild(leftHandContent, leftHandFormControls);
  const rightHandFormControls = document.createElement("div");
  adoptChild(rightHandContent, rightHandFormControls);

  const createAccountControls = document.createElement("section");
  adoptChild(leftHandFormControls, createAccountControls);
  const createAccountTitle = document.createElement("div");
  adoptChild(createAccountControls, createAccountTitle);
  createAccountTitle.textContent = "Create Account";
  createAccountTitle.style.fontSize = "24px";
  createAccountTitle.style.fontWeight = "bold";
  const newAccountNameLabel = document.createElement("label");
  adoptChild(createAccountControls, newAccountNameLabel);
  newAccountNameLabel.textContent = "name";
  const newAccountNameInput = document.createElement("input");
  adoptChild(newAccountNameLabel, newAccountNameInput);
  newAccountNameInput.type = "text";
  newAccountNameInput.style.margin = "1ch";
  const createAccountButton = document.createElement("button");
  adoptChild(createAccountControls, createAccountButton);
  createAccountButton.textContent = "create";

  const addTransactionControls = document.createElement("section");
  adoptChild(rightHandFormControls, addTransactionControls);
  const addTransactionTitle = document.createElement("div");
  adoptChild(addTransactionControls, addTransactionTitle);
  addTransactionTitle.textContent = "Add Transaction to Account";
  addTransactionTitle.style.fontSize = "24px";
  addTransactionTitle.style.fontWeight = "bold";

  addTransactionControls.style.display = "flex";
  addTransactionControls.style.flexDirection = "column";
  addTransactionControls.style.alignItems = "flex-start";
  const addTransactionDescriptionLabel = document.createElement("label");
  adoptChild(addTransactionControls, addTransactionDescriptionLabel);
  addTransactionDescriptionLabel.textContent = "description";
  const addTransactionDescriptionInput = document.createElement("input");
  adoptChild(addTransactionDescriptionLabel, addTransactionDescriptionInput);
  addTransactionDescriptionInput.type = "text";
  addTransactionDescriptionInput.style.margin = "1ch";
  const addTransactionAmountLabel = document.createElement("label");
  adoptChild(addTransactionControls, addTransactionAmountLabel);
  addTransactionAmountLabel.textContent = "amount";
  const addTransactionAmountInput = document.createElement("input");
  adoptChild(addTransactionAmountLabel, addTransactionAmountInput);
  addTransactionAmountInput.type = "number";
  addTransactionAmountInput.min = "0";
  addTransactionAmountInput.step = "any";
  addTransactionAmountInput.style.margin = "1ch";
  const addTransactionDateLabel = document.createElement("label");
  adoptChild(addTransactionControls, addTransactionDateLabel);
  addTransactionDateLabel.textContent = "date";
  const addTransactionDateInput = document.createElement("input");
  adoptChild(addTransactionDateLabel, addTransactionDateInput);
  addTransactionDateInput.type = "date";
  addTransactionDateInput.style.margin = "1ch";
  const addTransactionButton = document.createElement("button");
  adoptChild(addTransactionControls, addTransactionButton);
  addTransactionButton.textContent = "add";

  const transferAndAllocateControls = document.createElement("section");
  adoptChild(leftHandFormControls, transferAndAllocateControls);
  const transferAndAllocateTitle = document.createElement("div");
  adoptChild(transferAndAllocateControls, transferAndAllocateTitle);
  transferAndAllocateTitle.textContent = "Transfer to/Allocate Account";
  transferAndAllocateTitle.style.fontSize = "24px";
  transferAndAllocateTitle.style.fontWeight = "bold";

  const transferAndAllocateAmountLabel = document.createElement("label");
  adoptChild(transferAndAllocateControls, transferAndAllocateAmountLabel);
  transferAndAllocateAmountLabel.textContent = "amount";
  const transferAndAllocateInput = document.createElement("input");
  adoptChild(transferAndAllocateAmountLabel, transferAndAllocateInput);
  transferAndAllocateInput.type = "number";
  transferAndAllocateInput.min = "0";
  transferAndAllocateInput.step = "any";
  transferAndAllocateInput.style.margin = "1ch";
  const transferButton = document.createElement("button");
  adoptChild(transferAndAllocateControls, transferButton);
  transferButton.textContent = "transfer";
  const allocateButton = document.createElement("button");
  adoptChild(transferAndAllocateControls, allocateButton);
  allocateButton.textContent = "allocate";

  let selectedAccountTransactionTableBody: HTMLTableSectionElement | null =
    null;
  let selectedTransactionRow: HTMLTableRowElement | null = null;
  let selectedAccountSummaryRow: HTMLTableRowElement | null = null;
  const accountTableBodies: HTMLTableSectionElement[] = [];

  function transactionRowSelectionHandler(row: HTMLTableRowElement) {
    return () => {
      if (selectedTransactionRow !== null) {
        selectedTransactionRow.style.backgroundColor = "";
      }
      row.style.backgroundColor = "#8cb4ff";
      selectedTransactionRow = row;
    };
  }

  const websocket = new WebSocket(`ws://${window.location.host}`);
  websocket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    switch (message.method) {
      case "mark as saved": {
        saveButton.style.backgroundColor = "green";
        break;
      }
      case "mark as unsaved": {
        saveButton.style.backgroundColor = "red";
        break;
      }
      case "update net income": {
        netIncome.textContent = message.amount;
        break;
      }
      case "add account table": {
        const row = accountSummaryTableBody.insertRow(
          message.accountIndex >= accountSummaryTableBody.rows.length
            ? -1
            : message.accountIndex
        );
        createChild(row, "td");
        const name = document.createElement("td");
        adoptChild(row, name);
        name.textContent = message.name;
        createChild(row, "td").style.textAlign = "right";
        createChild(row, "td").style.textAlign = "right";

        const transactionTableBody = document.createElement("tbody");
        adoptChild(transactionTable, transactionTableBody);
        accountTableBodies.splice(
          message.accountIndex,
          0,
          transactionTableBody
        );
        transactionTableBody.style.display = "none";

        row.addEventListener("click", () => {
          if (selectedAccountSummaryRow !== null) {
            selectedAccountSummaryRow.style.backgroundColor = "";
          }
          row.style.backgroundColor = "#8cb4ff";
          if (selectedAccountTransactionTableBody !== null) {
            selectedAccountTransactionTableBody.style.display = "none";
          }
          transactionTableBody.style.display = "";
          rightHandTableTitle.textContent = accountName(row);
          selectedAccountTransactionTableBody = transactionTableBody;
          selectedAccountSummaryRow = row;
        });
        break;
      }
      case "delete account table": {
        const [body] = accountTableBodies.splice(message.accountIndex, 1);
        body.parentNode!.removeChild(body);
        accountSummaryTableBody.deleteRow(message.accountIndex);
        break;
      }
      case "add transaction row": {
        const parent = accountTableBody(accountTableBodies, message);
        const row = parent.insertRow(
          message.transactionIndex >= parent.rows.length
            ? -1
            : message.transactionIndex
        );
        createChild(row, "td"), createChild(row, "td");
        createChild(row, "td").style.textAlign = "right";
        createChild(row, "td").style.textAlign = "center";
        createChild(row, "td").style.textAlign = "center";
        row.onclick = transactionRowSelectionHandler(row);
        updateTransaction(row, message);
        break;
      }
      case "delete transaction row":
        accountTableBody(accountTableBodies, message).deleteRow(
          message.transactionIndex
        );
        break;
      case "update account balance":
        accountSummaryRow(
          accountSummaryTableBody,
          message
        ).lastElementChild!.textContent = message.amount;
        break;
      case "update account allocation":
        accountSummaryRow(
          accountSummaryTableBody,
          message
        ).cells[2].textContent = message.amount;
        break;
      case "check transaction row":
        transactionRow(accountTableBodies, message).cells[4].textContent = "✅";
        break;
      case "remove transaction row selection": {
        const row = transactionRow(accountTableBodies, message);
        row.style.color = "grey";
        row.onclick = null;
        break;
      }
      default:
        break;
    }
  };
  sendOnClick(saveButton, websocket, () => ({
    method: "save",
  }));
  sendOnClick(reduceButton, websocket, () => ({
    method: "reduce",
  }));
  sendOnClick(restoreButton, websocket, () => ({
    method: "restore",
  }));
  sendOnClick(
    removeAccountButton,
    websocket,
    () => ({
      method: "remove account",
      name: accountName(selectedAccountSummaryRow!),
    }),
    () => {
      return selectedAccountSummaryRow !== null;
    }
  );
  sendOnClick(
    closeAccountButton,
    websocket,
    () => ({
      method: "close account",
      name: accountName(selectedAccountSummaryRow!),
    }),
    () => {
      return selectedAccountSummaryRow !== null;
    }
  );
  sendOnClick(
    removeTransactionButton,
    websocket,
    () =>
      transactionMessage(
        selectedAccountSummaryRow!,
        selectedTransactionRow!,
        "remove transaction"
      ),
    () => {
      return (
        selectedAccountSummaryRow !== null && selectedTransactionRow !== null
      );
    }
  );
  sendOnClick(
    verifyTransactionButton,
    websocket,
    () =>
      transactionMessage(
        selectedAccountSummaryRow!,
        selectedTransactionRow!,
        "verify transaction"
      ),
    () => {
      return (
        selectedAccountSummaryRow !== null && selectedTransactionRow !== null
      );
    }
  );
  createAccountButton.addEventListener("click", () => {
    sendMessage(websocket, {
      method: "create account",
      name: newAccountNameInput.value,
    });
    newAccountNameInput.value = "";
  });
  transferButton.addEventListener("click", () => {
    if (selectedAccountSummaryRow !== null) {
      sendMessage(websocket, {
        method: "transfer",
        name: accountName(selectedAccountSummaryRow),
        amount: transferAndAllocateInput.value,
      });
      transferAndAllocateInput.value = "";
    }
  });
  allocateButton.addEventListener("click", () => {
    if (selectedAccountSummaryRow !== null) {
      sendMessage(websocket, {
        method: "allocate",
        name: accountName(selectedAccountSummaryRow),
        amount: transferAndAllocateInput.value,
      });
      transferAndAllocateInput.value = "";
    }
  });
  addTransactionButton.addEventListener("click", () => {
    if (selectedAccountSummaryRow !== null) {
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
    }
  });
}

document.addEventListener("DOMContentLoaded", () => {
  main();
});
