const websocket = new WebSocket("ws://localhost:9012");
websocket.onmessage = (event) => {
  const message = JSON.parse(event.data);
  switch (message.method) {
    case "add account": {
      const account = document.createElement("table");
      account.id = message.name;
      const header = document.createElement("tr");
      const amount = document.createElement("th");
      amount.textContent = "Amount (USD)";
      header.append(amount);
      const date = document.createElement("th");
      date.textContent = "Date";
      header.append(date);
      const description = document.createElement("th");
      description.textContent = "Description";
      header.append(description);
      account.append(header);
      document.body.append(account);
      break;
    }
    case "remove account":
      break;
    case "update balance":
      break;
    case "add credit": {
      const account = document.getElementById(message.name);
      const transaction = document.createElement("tr");
      const amount = document.createElement("td");
      amount.textContent = message.amount;
      transaction.append(amount);
      const date = document.createElement("td");
      date.textContent = message.date;
      transaction.append(date);
      const description = document.createElement("td");
      description.textContent = message.description;
      transaction.append(description);
      account.append(transaction);
      break;
    }
    case "remove credit":
      break;
    case "add debit":
      break;
    case "remove debit":
      break;
    case "update account balance":
      break;
    default:
      break;
  }
};
