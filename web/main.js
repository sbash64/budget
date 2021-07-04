function createChild(parent, tagName) {
  const child = document.createElement(tagName);
  parent.append(child);
  return child;
}

function initializeAccountTable(accountTable) {
  const header = createChild(accountTable, "tr");
  createChild(header, "th").textContent = "Name";
  createChild(header, "th").textContent = "Balance";
}

function main() {
  const accountsWithSummaries = createChild(document.body, "div");
  accountsWithSummaries.style.display = "flex";
  const accountTable = createChild(accountsWithSummaries, "table");
  const totalBalance = createChild(document.body, "div");
  const controls = createChild(document.body, "div");
  controls.style.display = "flex";
  const descriptionInput = createChild(controls, "input");
  descriptionInput.type = "text";
  const amountInput = createChild(controls, "input");
  amountInput.type = "number";
  amountInput.min = 0;
  amountInput.step = "any";
  const dateInput = createChild(controls, "input");
  dateInput.type = "date";
  const addTransactionButton = createChild(controls, "button");
  initializeAccountTable(accountTable);
  let accountShowing = null;
  const accounts = new Map();
  const accountSummaries = new Map();
  const accountNames = new Map();
  const websocket = new WebSocket("ws://localhost:9012");
  websocket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    switch (message.method) {
      case "add account": {
        const accountSummary = createChild(accountTable, "tr");
        const name = createChild(accountSummary, "td");
        name.textContent = message.name;
        createChild(accountSummary, "td");
        accountSummaries.set(message.name, accountSummary);

        const account = createChild(accountsWithSummaries, "table");
        const header = createChild(account, "tr");
        createChild(header, "th").textContent = "Description";
        createChild(header, "th").textContent = "Debits";
        createChild(header, "th").textContent = "Credits";
        createChild(header, "th").textContent = "Date";
        accounts.set(message.name, account);
        accountNames.set(account, message.name);

        account.style.display = "none";
        accountSummary.addEventListener("mouseup", () => {
          if (accountShowing) accountShowing.style.display = "none";
          account.style.display = "block";
          accountShowing = account;
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
        createChild(transaction, "td").textContent = message.description;
        createChild(transaction, "td");
        createChild(transaction, "td").textContent = message.amount;
        createChild(transaction, "td").textContent = message.date;
        break;
      }
      case "remove credit": {
        const account = accounts.get(message.name);
        for (let i = 0; i < account.rows.length; i += 1) {
          if (
            account.rows[i].cells[0].textContent === message.description &&
            account.rows[i].cells[2].textContent === message.amount &&
            account.rows[i].cells[3].textContent === message.date
          ) {
            account.deleteRow(i);
            break;
          }
        }
        break;
      }
      case "add debit": {
        const transaction = createChild(accounts.get(message.name), "tr");
        createChild(transaction, "td").textContent = message.description;
        createChild(transaction, "td").textContent = message.amount;
        createChild(transaction, "td");
        createChild(transaction, "td").textContent = message.date;
        break;
      }
      case "remove debit": {
        const account = accounts.get(message.name);
        for (let i = 0; i < account.rows.length; i += 1) {
          if (
            account.rows[i].cells[0].textContent === message.description &&
            account.rows[i].cells[1].textContent === message.amount &&
            account.rows[i].cells[3].textContent === message.date
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
  addTransactionButton.addEventListener("mouseup", () => {
    websocket.send(
      JSON.stringify({
        method: "debit",
        name: accountNames.get(accountShowing),
        description: descriptionInput.value,
        amount: amountInput.value,
        date: dateInput.value,
      })
    );
  });
}

document.addEventListener("DOMContentLoaded", () => {
  main();
});
