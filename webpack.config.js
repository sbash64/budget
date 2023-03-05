const HtmlWebpackPlugin = require("html-webpack-plugin");

module.exports = {
  entry: "./web/main.js",
  plugins: [new HtmlWebpackPlugin({ title: "Bashford Budget" })],
};
