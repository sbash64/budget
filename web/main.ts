import "./styles.css";

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
  const page = createChild(document.body, "div");
  page.style.display = "grid";
  const topPage = createChild(page, "div");
  topPage.style.gridRow = "1";
  topPage.style.display = "grid";

  const netIncomeLabel = createChild(topPage, "label");
  netIncomeLabel.textContent = "Net Income";
  netIncomeLabel.style.gridRow = "1";

  const netIncome = createChild(netIncomeLabel, "strong");
  netIncome.style.margin = "1ch";

  const topPageButtons = createChild(topPage, "div");
  topPageButtons.style.gridRow = "2";

  const saveButton = createChild(topPageButtons, "button");
  saveButton.textContent = "save";
  const reduceButton = createChild(topPageButtons, "button");
  reduceButton.textContent = "reduce";
  const restoreButton = createChild(topPageButtons, "button");
  restoreButton.textContent = "restore";

  const pageBody = createChild(page, "div");
  pageBody.style.display = "grid";
  pageBody.style.gridRow = "2";

  const tableViews = createChild(pageBody, "div");
  tableViews.style.gridRow = "2";
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
  leftHandTableView.style.gridRow = "1";

  const rightHandTableViewButtons = createChild(rightHandTableView, "div");
  rightHandTableViewButtons.style.gridRow = "1";

  const accountSummaryTableWrapper = createChild(leftHandTableView, "div");
  accountSummaryTableWrapper.style.gridRow = "2";
  accountSummaryTableWrapper.style.height = "20em";
  accountSummaryTableWrapper.style.overflowY = "scroll";

  const transactionTableWrapper = createChild(rightHandTableView, "div");
  transactionTableWrapper.style.gridRow = "2";
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
  const accountSummaryTableBody = createChild(
    accountSummaryTable,
    "tbody"
  ) as HTMLTableSectionElement;

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
  formControls.style.gridRow = "3";
  formControls.style.display = "grid";

  const leftHandFormControls = createChild(formControls, "div");
  leftHandFormControls.style.gridColumn = "1";
  const rightHandFormControls = createChild(formControls, "div");
  rightHandFormControls.style.gridColumn = "2";

  const createAccountControls = createChild(leftHandFormControls, "section");
  createChild(createAccountControls, "h4").textContent = "Create Account";
  const newAccountNameLabel = createChild(createAccountControls, "label");
  newAccountNameLabel.textContent = "name";
  const newAccountNameInput = createChild(
    newAccountNameLabel,
    "input"
  ) as HTMLInputElement;
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
  ) as HTMLInputElement;
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
  ) as HTMLInputElement;
  addTransactionAmountInput.type = "number";
  addTransactionAmountInput.min = "0";
  addTransactionAmountInput.step = "any";
  addTransactionAmountInput.style.margin = "1ch";
  const addTransactionDateLabel = createChild(addTransactionControls, "label");
  addTransactionDateLabel.textContent = "date";
  const addTransactionDateInput = createChild(
    addTransactionDateLabel,
    "input"
  ) as HTMLInputElement;
  addTransactionDateInput.type = "date";
  addTransactionDateInput.style.margin = "1ch";
  const addTransactionButton = createChild(addTransactionControls, "button");
  addTransactionButton.textContent = "add";

  const transferAndAllocateControls = createChild(
    leftHandFormControls,
    "section"
  );
  createChild(transferAndAllocateControls, "h4").textContent =
    "Transfer to/Allocate Account";
  transferAndAllocateControls.style.display = "flex";
  transferAndAllocateControls.style.flexDirection = "column";
  transferAndAllocateControls.style.alignItems = "flex-start";
  const transferAndAllocateAmountLabel = createChild(
    transferAndAllocateControls,
    "label"
  );
  transferAndAllocateAmountLabel.textContent = "amount";
  const transferAndAllocateInput = createChild(
    transferAndAllocateAmountLabel,
    "input"
  ) as HTMLInputElement;
  transferAndAllocateInput.type = "number";
  transferAndAllocateInput.min = "0";
  transferAndAllocateInput.step = "any";
  transferAndAllocateInput.style.margin = "1ch";
  const transferButton = createChild(transferAndAllocateControls, "button");
  transferButton.textContent = "transfer";
  const allocateButton = createChild(transferAndAllocateControls, "button");
  allocateButton.textContent = "allocate";

  let selectedAccountTransactionTableBody: HTMLTableSectionElement | null =
    null;
  let selectedTransactionRow: HTMLTableRowElement | null = null;
  let selectedAccountSummaryRow: HTMLTableRowElement | null = null;
  const accountTableBodies: HTMLTableSectionElement[] = [];

  function transactionRowSelectionHandler(row: HTMLTableRowElement) {
    return () => {
      if (selectedTransactionRow !== null) {
        selectedTransactionRow.style.color = "";
        selectedTransactionRow.style.backgroundColor = "";
      }
      row.style.color = "white";
      row.style.backgroundColor = "blue";
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
        const name = createChild(row, "td");
        name.textContent = message.name;
        createChild(row, "td").style.textAlign = "right";
        createChild(row, "td").style.textAlign = "right";

        const transactionTableBody = createChild(
          transactionTable,
          "tbody"
        ) as HTMLTableSectionElement;
        accountTableBodies.splice(
          message.accountIndex,
          0,
          transactionTableBody
        );
        transactionTableBody.style.display = "none";

        row.addEventListener("click", () => {
          if (selectedAccountSummaryRow !== null) {
            selectedAccountSummaryRow.style.backgroundColor = "";
            selectedAccountSummaryRow.style.color = "";
          }
          row.style.backgroundColor = "blue";
          row.style.color = "white";
          if (selectedAccountTransactionTableBody !== null) {
            selectedAccountTransactionTableBody.style.display = "none";
          }
          transactionTableBody.style.display = "";
          rightHandContentHeader.textContent = accountName(row);
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
