var suites = [
  "UiProtocolTests.js",
  "UiBindingTests.js",
  "RegistryAndBankManagerTests.js",
  "LiveInstanceHostTests.js",
  "RuntimePathTests.js",
];
suites.forEach(function (suite) {
  require("./" + suite);
});
console.log("ClientTests passed");
