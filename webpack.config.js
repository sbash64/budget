const HtmlWebpackPlugin = require("html-webpack-plugin");

module.exports = {
  entry: "./web/main.ts",
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        use: "ts-loader",
        exclude: /node_modules/,
      },
    ],
  },
  plugins: [new HtmlWebpackPlugin({ title: "Bashford Budget" })],
};
