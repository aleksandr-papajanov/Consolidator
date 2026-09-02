var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");
var createRequire = require("module").createRequire;
var environment = require("../../support/ProductionEnvironment.js");
var root = environment.root;
environment.loadClientEnvironment();

function loadMaxClass(relativePath, className) {
  var absolutePath = path.join(root, relativePath);
  var source = fs.readFileSync(absolutePath, "utf8");
  var factory = vm.runInThisContext(
    "(function (require, mgraphics, outlet) {\n" +
      source + "\nreturn " + className + ";\n})",
    { filename: relativePath },
  );
  return factory(createRequire(absolutePath), global.mgraphics, global.outlet);
}

function testMultiValueTogglePresenterPublishesOptionsAndWritesIndex() {
  var MultiValueTogglePresenter = require(
    path.join(root, "js/Presenters/MultiValueToggle/MultiValueTogglePresenter.js")
  ).MultiValueTogglePresenter;
  var writes = [];
  var source = {
    value: 1,
    enabled: true,
    subscribe: function () { return function () {}; },
    set: function (value) { writes.push(value); },
  };
  var presenter = new MultiValueTogglePresenter({
    value: source,
    values: ["Punch", "Tight", "Smooth"],
  });

  assert.deepStrictEqual(presenter.presentation.values,
    ["Punch", "Tight", "Smooth"]);
  assert.strictEqual(presenter.presentation.value, 1);
  presenter.setValue(2);
  assert.deepStrictEqual(writes, [2]);
  presenter.destroy();
}

function testMultiValueToggleControlCyclesAndEmitsSelectedIndex() {
  var emissions = [];
  global.outlet = function (outletIndex, values) {
    emissions.push([outletIndex, values]);
  };
  var MultiValueToggleControl = loadMaxClass(
    "js/Controls/MultiValueToggle/MultiValueToggleControl.js",
    "MultiValueToggleControl",
  );
  var control = new MultiValueToggleControl();
  control.values = ["Punch", "Tight", "Smooth"];
  control.value = 1;
  control.beginGesture(0);
  control.drag(-34);

  assert.deepStrictEqual(emissions, [[0, ["valueChanged", 2]]]);
}

testMultiValueTogglePresenterPublishesOptionsAndWritesIndex();
testMultiValueToggleControlCyclesAndEmitsSelectedIndex();
console.log("MultiValueToggleTests passed");
