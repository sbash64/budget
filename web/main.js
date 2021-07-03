function createChild(parent, tagName) {
  const child = document.createElement(tagName);
  parent.append(child);
  return child;
}

function initializeAccounts(accountTable) {
  const header = createChild(accountTable, "tr");
  const name = createChild(header, "th");
  name.textContent = "Name";
  const balance = createChild(header, "th");
  balance.textContent = "Balance";
}

function main() {
  const accountTable = createChild(document.body, "table");
  initializeAccounts(accountTable);
  const accounts = new Map();
  const accountSummaries = new Map();
  const websocket = new WebSocket("ws://localhost:9012");
  let accountShowing = null;
  websocket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    switch (message.method) {
      case "add account": {
        const accountSummary = createChild(accountTable, "tr");
        const name = createChild(accountSummary, "td");
        name.textContent = message.name;
        createChild(accountSummary, "td");
        accountSummaries.set(message.name, accountSummary);

        const account = createChild(document.body, "table");
        const header = createChild(account, "tr");
        const description = createChild(header, "th");
        description.textContent = "Description";
        const debits = createChild(header, "th");
        debits.textContent = "Debits";
        const credits = createChild(header, "th");
        credits.textContent = "Credits";
        const date = createChild(header, "th");
        date.textContent = "Date";
        accounts.set(message.name, account);

        account.style.display = "none";
        accountSummary.addEventListener("mouseup", () => {
          if (accountShowing) accountShowing.style.display = "none";
          account.style.display = "block";
          accountShowing = account;
        });
        break;
      }
      case "remove account":
        break;
      case "update balance":
        break;
      case "add credit": {
        const account = accounts.get(message.name);
        const transaction = createChild(account, "tr");
        const description = createChild(transaction, "td");
        description.textContent = message.description;
        transaction.append(document.createElement("td"));
        const amount = createChild(transaction, "td");
        amount.textContent = message.amount;
        const date = createChild(transaction, "td");
        date.textContent = message.date;
        break;
      }
      case "remove credit":
        break;
      case "add debit": {
        const account = accounts.get(message.name);
        const transaction = createChild(account, "tr");
        const description = createChild(transaction, "td");
        description.textContent = message.description;
        const amount = createChild(transaction, "td");
        amount.textContent = message.amount;
        transaction.append(document.createElement("td"));
        const date = createChild(transaction, "td");
        date.textContent = message.date;
        break;
      }
      case "remove debit":
        break;
      case "update account balance": {
        const accountSummary = accountSummaries.get(message.name);
        accountSummary.lastElementChild.textContent = message.amount;
        break;
      }
      default:
        break;
    }
  };
}

document.addEventListener("DOMContentLoaded", () => {
  main();
});
