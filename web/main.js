function initializeAccounts(accountTable) {
  const header = document.createElement("tr");
  const name = document.createElement("th");
  name.textContent = "Name";
  header.append(name);
  const balance = document.createElement("th");
  balance.textContent = "Balance";
  header.append(balance);
  accountTable.append(header);
}

function main() {
  const accountTable = document.createElement("table");
  initializeAccounts(accountTable);
  document.body.append(accountTable);
  const accounts = new Map();
  const accountSummaries = new Map();
  const websocket = new WebSocket("ws://localhost:9012");
  let accountShowing = null;
  websocket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    switch (message.method) {
      case "add account": {
        const accountSummary = document.createElement("tr");
        const name = document.createElement("td");
        accountSummary.append(name);
        name.textContent = message.name;
        const balance = document.createElement("td");
        accountSummary.append(balance);
        accountTable.append(accountSummary);
        accountSummaries.set(message.name, accountSummary);

        const account = document.createElement("table");
        const header = document.createElement("tr");
        const description = document.createElement("th");
        description.textContent = "Description";
        header.append(description);
        const debits = document.createElement("th");
        debits.textContent = "Debits";
        header.append(debits);
        const credits = document.createElement("th");
        credits.textContent = "Credits";
        header.append(credits);
        const date = document.createElement("th");
        date.textContent = "Date";
        header.append(date);
        account.append(header);
        document.body.append(account);
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
        const transaction = document.createElement("tr");
        const description = document.createElement("td");
        description.textContent = message.description;
        transaction.append(description);
        transaction.append(document.createElement("td"));
        const amount = document.createElement("td");
        amount.textContent = message.amount;
        transaction.append(amount);
        const date = document.createElement("td");
        date.textContent = message.date;
        transaction.append(date);
        account.append(transaction);
        break;
      }
      case "remove credit":
        break;
      case "add debit": {
        const account = accounts.get(message.name);
        const transaction = document.createElement("tr");
        const description = document.createElement("td");
        description.textContent = message.description;
        transaction.append(description);
        const amount = document.createElement("td");
        amount.textContent = message.amount;
        transaction.append(amount);
        transaction.append(document.createElement("td"));
        const date = document.createElement("td");
        date.textContent = message.date;
        transaction.append(date);
        account.append(transaction);
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
