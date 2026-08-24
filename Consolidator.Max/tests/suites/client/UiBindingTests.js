var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");
var environment = require("../../support/ProductionEnvironment.js");
var stateFixtures = require("../../support/StateFixtures.js");
var root = environment.root;
environment.loadClientEnvironment();
var makeStateFixture = stateFixtures.makeStateFixture;
function testControlBindingsDispatchByControlId() {
  var calls = [];
  var bindings = new ControlBindings();
  bindings.add("compressor_threshold", {
    handleIntent: function (intent, values) {
      calls.push([intent, values]);
    },
    destroy: function () {},
  });

  bindings.handle("compressor_threshold", "valueChanged", [0, 0.5]);
  bindings.handle("compressor_ratio", "valueChanged", [0, 4]);

  assert.deepStrictEqual(calls, [["valueChanged", [0, 0.5]]]);
  assert.throws(function () {
    bindings.add("compressor_threshold", {
      handleIntent: function () {},
      destroy: function () {},
    });
  }, /Duplicate control binding varname/);
  bindings.destroy();
  assert.deepStrictEqual(bindings.items, {});
}
function testDialBindingUsesMessageTransportAndIntents() {
  var messages = [];
  var intents = [];
  var presentation = {
    enabled: true,
    active: true,
    activeIndex: 0,
    displayIndex: 0,
    rings: [{ minimum: 0, maximum: 1, value: 0.5 }],
  };
  var presenter = {
    subscribe: function (callback, immediate) {
      if (immediate) callback(presentation);
      return function () {};
    },
    setValue: function (index, value) {
      intents.push(["setValue", index, value]);
    },
    resetValue: function (index) {
      intents.push(["resetValue", index]);
    },
    beginGesture: function (index) {
      intents.push(["beginGesture", index]);
    },
    endGesture: function (index) {
      intents.push(["endGesture", index]);
    },
  };
  var binding = new DialControlBinding(presenter, function (name, args) {
    messages.push([name, args]);
  });

  assert.deepStrictEqual(messages, [
    ["enabled", [1]],
    ["active", [1]],
    ["activeIndex", [0]],
    ["displayIndex", [0]],
    ["ringCount", [1]],
    ["limits", [0, 0, 1]],
    ["set", [0, 0.5]],
  ]);
  binding.handleIntent("valueChanged", [0, 0.75]);
  binding.handleIntent("reset", [0]);
  binding.handleIntent("gestureBegan", [0]);
  binding.handleIntent("gestureEnded", [0]);
  assert.deepStrictEqual(intents, [
    ["setValue", 0, 0.75],
    ["resetValue", 0],
    ["beginGesture", 0],
    ["endGesture", 0],
  ]);
  binding.destroy();
}

function testDialBindingWrapsGesturesInTransactions() {
  var transactionCalls = [];
  var intents = [];
  var beginCallback = null;
  var presenter = {
    subscribe: function () { return function () {}; },
    setValue: function (index, value, transactionId) {
      intents.push(["setValue", index, value, transactionId]);
    },
    beginGesture: function (index, transactionId) {
      intents.push(["beginGesture", index, transactionId]);
    },
    endGesture: function (index, transactionId) {
      intents.push(["endGesture", index, transactionId]);
    },
  };
  var transactions = {
    begin: function (callback) {
      transactionCalls.push(["begin"]);
      beginCallback = callback;
      return 9;
    },
    end: function (id) {
      transactionCalls.push(["end", id]);
    },
  };
  var binding = new DialControlBinding(presenter, function () {}, transactions);

  binding.handleIntent("gestureBegan", [0]);
  binding.handleIntent("valueChanged", [0, 0.75]);
  binding.handleIntent("gestureEnded", [0]);
  beginCallback(9, { status: "accepted" });

  assert.deepStrictEqual(transactionCalls, [["begin"], ["end", 9]]);
  assert.deepStrictEqual(intents, [
    ["beginGesture", 0, 9],
    ["setValue", 0, 0.75, 9],
    ["endGesture", 0, 9],
  ]);
  binding.destroy();
}

function testDialBindingReportsRejectedTransactionWithoutWriting() {
  var messages = [];
  var writes = [];
  var beginCallback = null;
  var presenter = {
    subscribe: function () { return function () {}; },
    setValue: function () { writes.push(Array.prototype.slice.call(arguments)); },
    beginGesture: function () {},
    endGesture: function () {},
  };
  var transactions = {
    begin: function (callback) {
      beginCallback = callback;
      return 10;
    },
    end: function () {
      throw new Error("Rejected transaction must not end.");
    },
  };
  var binding = new DialControlBinding(presenter, function (name) {
    messages.push(name);
  }, transactions);

  binding.handleIntent("gestureBegan", [0]);
  binding.handleIntent("valueChanged", [0, 0.5]);
  beginCallback(10, { status: "rejected" });

  assert.deepStrictEqual(writes, []);
  assert.deepStrictEqual(messages, ["transactionRejected"]);
  binding.destroy();
}
function testButtonBindingPreservesPresentationMetadata() {
  var messages = [];
  var presenter = {
    subscribe: function (callback, immediate) {
      if (immediate)
        callback({
          value: true,
          enabled: true,
          active: true,
          mode: "momentary",
          label: "SOLO",
        });
      return function () {};
    },
  };
  var binding = new ButtonControlBinding(presenter, function (name, args) {
    messages.push([name, args]);
  });

  assert.deepStrictEqual(messages, [
    ["set", [1]],
    ["enabled", [1]],
    ["active", [1]],
    ["mode", ["momentary"]],
    ["label", ["SOLO"]],
  ]);
  binding.destroy();
}
function testUiHostRoutesIntentsByControlVarname() {
  var calls = [];
  var host = Object.create(ConsolidatorUiHost.prototype);
  host.bindings = new ControlBindings();
  host.sendControlMessage = function () {};
  host.bind("input_gain", function () {
    return {
      handleIntent: function (name, values) {
        calls.push([name, values]);
      },
      destroy: function () {},
    };
  });

  host.handleUiIntent("input_gain", "valueChanged", [0, 0.75]);
  assert.deepStrictEqual(calls, [["valueChanged", [0, 0.75]]]);
  host.bindings.destroy();
}
function testPanelBindingHostRoutesListMessagesToNamedControl() {
  var calls = [];
  var context = vm.createContext({
    patcher: {
      getnamed: function (name) {
        assert.strictEqual(name, "input_gain");
        return {
          message: function () {
            calls.push(Array.prototype.slice.call(arguments));
          },
        };
      },
    },
    arrayfromargs: function (values) {
      return Array.prototype.slice.call(values);
    },
  });
  vm.runInContext(
    fs.readFileSync(path.join(root, "js/PanelBindingHost.js"), "utf8"),
    context,
    { filename: "js/PanelBindingHost.js" },
  );

  context.list("input_gain", "ringCount", 1);
  context.list("input_gain", "set", 0, 0.5);

  assert.deepStrictEqual(calls, [["ringCount", 1], ["set", 0, 0.5]]);
}
function testAnalyzerHandlesPublishOnlyAfterTargetSnapshotIsReady() {
  var statusCallback = null;
  var sourceListeners = [];
  var frequency = {
    value: 1000,
    subscribe: function (callback) {
      sourceListeners.push(callback);
      return function () {};
    },
  };
  var writes = [];
  var presenter = new AnalyzerPresenter({
    mode: "equalizer",
    gainRange: { minimum: -24, maximum: 24 },
    statusSource: {
      subscribeStatus: function (callback, immediate) {
        statusCallback = callback;
        if (immediate) callback({ ready: false, loading: true });
        return function () {};
      },
    },
    parameters: [{
      frequency: frequency,
      gain: { value: 3 },
      q: { value: 1, minimum: 0.1, maximum: 10 },
      enabled: true,
      setPosition: function (nextFrequency, nextGain, transactionId) {
        writes.push([nextFrequency, nextGain, transactionId]);
      },
    }],
  });

  assert.strictEqual(presenter.presentation.enabled, false);
  assert.deepStrictEqual(presenter.presentation.handles, []);
  frequency.value = 2000;
  sourceListeners[0]();
  assert.deepStrictEqual(presenter.presentation.handles, []);

  statusCallback({ ready: true, loading: false });
  assert.strictEqual(presenter.presentation.enabled, true);
  assert.strictEqual(presenter.presentation.spectrum, null);
  assert.deepStrictEqual(presenter.presentation.curves, []);
  assert.strictEqual(presenter.presentation.handles.length, 1);
  presenter.filterMoved(1, 0.5, 0.25, 7);
  assert.strictEqual(writes.length, 1);
  assert.strictEqual(writes[0][2], 7);
  presenter.destroy();
}
function testAnalyzerBindingUsesOneTransactionForHandleDrag() {
  var calls = [];
  var transactionCalls = [];
  var beginCallback = null;
  var controller = {
    handle: function (intent, payload, transactionId) {
      calls.push([intent, payload, transactionId]);
    },
  };
  var presenter = {
    subscribe: function () {
      return function () {};
    },
  };
  var transactions = {
    begin: function (callback) {
      transactionCalls.push(["begin"]);
      beginCallback = callback;
      return 12;
    },
    end: function (id) {
      transactionCalls.push(["end", id]);
    },
  };
  var binding = new AnalyzerControlBinding(
    controller,
    presenter,
    function () {},
    transactions
  );

  binding.handleIntent("gestureBegan", [1]);
  binding.handleIntent("filterMoved", [1, 0.4, 0.6]);
  binding.handleIntent("filterMoved", [1, 0.5, 0.7]);
  binding.handleIntent("gestureEnded", [1]);
  beginCallback(12, { status: "accepted" });

  assert.deepStrictEqual(transactionCalls, [
    ["begin"],
    ["end", 12],
  ]);
  assert.deepStrictEqual(calls, [
    ["filterMoved", [1, 0.5, 0.7], 12],
  ]);
  binding.destroy();
}
function testAnalyzerHandleDragPublishesLatestPositionWhileDragging() {
  var messages = [];
  var scheduled = null;
  var context = vm.createContext({
    include: function () {},
    mgraphics: {
      init: function () {},
      redraw: function () {},
      relative_coords: 0,
      autofill: 0,
      size: [100, 100],
    },
    outlet: function (index, values) {
      messages.push(Array.prototype.slice.call(values));
    },
    arrayfromargs: function (values) {
      return Array.prototype.slice.call(values);
    },
    Task: function (callback) {
      this.schedule = function () {
        scheduled = callback;
      };
      this.cancel = function () {
        scheduled = null;
      };
    },
  });
  [
    "js/Controls/Analyzer/AnalyzerViewState.js",
    "js/Controls/Analyzer/AnalyzerLayout.js",
    "js/Controls/Analyzer/AnalyzerRenderer.js",
    "js/Controls/Analyzer/AnalyzerControl.js",
  ].forEach(function (file) {
    vm.runInContext(fs.readFileSync(path.join(root, file), "utf8"), context, {
      filename: file,
    });
  });
  context.analyzerControl.applyPresentation({
    mode: "equalizer",
    enabled: true,
    parameterRevision: 0,
    viewKey: "",
    handles: [{
      id: 1,
      frequency: 0.5,
      gain: 0.5,
      enabled: true,
      capabilities: { frequency: true, gain: true, q: true },
      selected: false,
    }],
  });

  context.onclick(52, 50);
  context.ondrag(60, 40, 1);
  context.ondrag(70, 30, 1);
  assert.deepStrictEqual(messages, [
    ["filterSelected", 1],
    ["gestureBegan", 1],
  ]);

  scheduled();
  scheduled = null;
  assert.strictEqual(messages.length, 3);
  assert.strictEqual(messages[2][0], "filterMoved");
  assert.strictEqual(messages[2][1], 1);
  assert.strictEqual(messages[2][2], (70 - 8) / 88);
  assert.strictEqual(messages[2][3], (30 - 8) / 84);

  context.ondrag(70, 30, 0);
  assert.deepStrictEqual(messages[3], ["gestureEnded", 1]);
}
function testUiHostAcceptsTrackNameMessage() {
  var state = makeStateFixture();
  var host = { client: { state: state } };
  ConsolidatorUiHost.prototype.setTrackName.call(host, [
    "name",
    '"Bass',
    'Player"',
  ]);

  assert.strictEqual(host.trackName, "Bass Player");
  assert.deepStrictEqual(state.sets, []);

  host.instanceId = "7";
  ConsolidatorUiHost.prototype.setTrackName.call(host, ["Drums"]);
  assert.strictEqual(host.trackName, "Drums");
  assert.deepStrictEqual(state.sets, [{
    instanceId: "7",
    path: "label",
    value: "Drums",
  }]);

  ConsolidatorUiHost.prototype.setTrackName.call(host, []);
  assert.strictEqual(host.trackName, "");
  assert.deepStrictEqual(state.sets[1], {
    instanceId: "7",
    path: "label",
    value: "",
  });
}
function testUiHostLoadsInIsolatedMaxContext() {
  var context = vm.createContext({
    console: console,
    setTimeout: setTimeout,
    clearTimeout: clearTimeout,
    jsarguments: ["ConsolidatorUiHost.js", "test.ui"],
    arrayfromargs: function (values) {
      return Array.prototype.slice.call(values);
    },
    outlet: function () {},
  });
  var includeStack = [];
  context.include = function (relativePath) {
    if (relativePath.indexOf("Project:/") === 0) {
      run(path.join(
        root,
        relativePath.substring("Project:/".length)
      ));
      return;
    }
    var base = includeStack.length
      ? path.dirname(includeStack[includeStack.length - 1])
      : path.join(root, "js");
    run(path.resolve(base, relativePath));
  };
  function run(file) {
    includeStack.push(file);
    vm.runInContext(fs.readFileSync(file, "utf8"), context, {
      filename: path.relative(root, file),
    });
    includeStack.pop();
  }

  run(path.join(root, "js/ConsolidatorUiHost.js"));
  var host = new context.ConsolidatorUiHost(
    "test.ui",
    function () {},
    function () {},
  );
  assert.strictEqual(host.lifecycle, "created");
  host.mapping = context.ConsolidatorControlMapping;
  host.bindControls();
  assert.deepStrictEqual(
    Object.keys(host.bindings.items).sort(),
    Object.keys(context.ConsolidatorControlMapping)
      .map(function (key) {
        return context.ConsolidatorControlMapping[key];
      })
      .sort(),
  );
  host.destroy();
  assert.strictEqual(host.lifecycle, "destroyed");
}
function testFeaturePresenterSetEnumeratesTypedPresenters() {
  var set = new FeaturePresenterSet();
  set.addDial("threshold", { value: 0.5 });
  set.addButton("bypass", { value: false });
  var entries = [];
  set.forEach(function (name, presenter, type) {
    entries.push([name, typeof presenter, type]);
  });

  assert.deepStrictEqual(entries, [
    ["threshold", "object", "dial"],
    ["bypass", "object", "button"],
  ]);
  set.destroy();
}
function testFeaturePresenterSetTracksSourceAvailability() {
  var listeners = [];
  var source = {
    value: 0.5,
    enabled: false,
    loading: true,
    subscribe: function (callback) {
      listeners.push(callback);
      return function () {};
    },
  };
  var set = new FeaturePresenterSet();
  var presenter = set.addDial("gain", source);

  assert.strictEqual(presenter.presentation.enabled, false);
  assert.strictEqual(presenter.presentation.loading, true);
  assert.strictEqual(listeners.length, 1);

  source.enabled = true;
  source.loading = false;
  listeners[0]();
  assert.strictEqual(presenter.presentation.enabled, true);
  assert.strictEqual(presenter.presentation.loading, false);
  set.destroy();
}
function testDialDisplayScaleDoesNotChangePhysicalValue() {
  var value = {
    value: 0.5,
    physicalMinimum: 0,
    physicalMaximum: 1,
    set: function (next) {
      this.value = next;
    },
  };
  var presenter = new DialPresenter({
    rings: [
      {
        value: value,
        display: { decimals: 1, suffix: "%", scale: 100 },
      },
    ],
  });

  assert.strictEqual(presenter.presentation.rings[0].display.value, "50.0%");
  presenter.setValue(0, 0.25);
  assert.strictEqual(value.value, 0.25);
  presenter.destroy();
}
function testConsolidatorInitializesDetectorState() {
  var state = makeStateFixture();
  var analysis = {
    subscribe: function () {
      return function () {};
    },
  };
  var root = new ConsolidatorViewModel(new UiTarget(state, analysis));
  var error = "not complete";
  root.initialize(function (result) {
    error = result;
  });
  assert.strictEqual(error, null);
  assert.strictEqual(state.paths.length, 0);
  assert.strictEqual(state.paths.length, 0);
  root.destroy();
}
function testFilterPositionUsesOneStateBatch() {
  var state = makeStateFixture();
  var filter = new FilterViewModel(state, 4);
  filter.setPosition(750, 3);
  assert.strictEqual(state.batches.length, 1);
  assert.deepStrictEqual(state.batches[0], [
    { path: "equalizer.filter.4.frequency", value: 750 },
    { path: "equalizer.filter.4.gain", value: 3 },
  ]);
  filter.destroy();
}
function testDetectorPositionUsesOneStateBatch() {
  var state = makeStateFixture();
  var filter = new DetectorFilterViewModel(state, "compressor", 2);
  filter.setPosition(1200, -3);
  assert.deepStrictEqual(state.batches[0], [
    { path: "compressor.detector.filter.2.frequency", value: 1200 },
    { path: "compressor.detector.filter.2.gain", value: -3 },
  ]);
  filter.destroy();
}
function testDetectorBypassIsInvertedForPresentation() {
  var state = makeStateFixture();
  var filter = new DetectorFilterViewModel(state, "compressor", 1);
  filter.bypass.value = false;
  assert.strictEqual(presentationBindingValue(filter.enabled), true);
  filter.bypass.value = true;
  assert.strictEqual(presentationBindingValue(filter.enabled), false);
  filter.destroy();
}
function testMessageControlsConstructCompletePresentation() {
  vm.runInThisContext(
    fs.readFileSync(path.join(root, "js/Controls/Dial/DialControl.js"), "utf8"),
    { filename: "js/Controls/Dial/DialControl.js" },
  );
  ringCount(1);
  limits(0, 0.1, 0.9);
  set(0, 0.4);
  assert.strictEqual(dialControl.presentation.rings.length, 1);
  assert.deepStrictEqual(dialControl.presentation.rings[0], {
    value: 0.4,
    minimum: 0.1,
    maximum: 0.9,
    visualization: null,
    color: null,
  });
  dialControl.dragging = true;
  dialControl.previewValues[0] = 0.8;
  transactionRejected();
  assert.strictEqual(dialControl.dragging, false);
  assert.deepStrictEqual(dialControl.previewValues, []);

  vm.runInThisContext(
    fs.readFileSync(
      path.join(root, "js/Controls/Button/ButtonControl.js"),
      "utf8",
    ),
    { filename: "js/Controls/Button/ButtonControl.js" },
  );
  active(1);
  mode("momentary");
  label("SOLO");
  assert.strictEqual(buttonControl.presentation.active, true);
  assert.strictEqual(buttonControl.presentation.mode, "momentary");
  assert.strictEqual(buttonControl.presentation.label, "SOLO");

  vm.runInThisContext(
    fs.readFileSync(
      path.join(root, "js/Presenters/BankManager/BankManagerPresentation.js"),
      "utf8",
    ),
    { filename: "js/Presenters/BankManager/BankManagerPresentation.js" },
  );
  vm.runInThisContext(
    fs.readFileSync(
      path.join(root, "js/Controls/BankManager/BankManagerControl.js"),
      "utf8",
    ),
    { filename: "js/Controls/BankManager/BankManagerControl.js" },
  );
  presentation_begin(1, 0);
  row(0, "instance.1", "Local", 1);
  bank(0, 1, "1", 1, 1, 1, 1, 0.75, 1, 0.1, 0.2, 0.3, 0.4, 0, 0, 0, 0, 0);
  link_group(2, "A", 1, 1, 1, 1, 0.5, 0.6, 0.7, 0.8);
  edit_action(1, 0);
  clear_action(1, 1);
  presentation_end();

  assert.strictEqual(bankManagerControl.pendingPresentation, null);
  assert.strictEqual(bankManagerControl.presentation.enabled, true);
  assert.strictEqual(bankManagerControl.presentation.linkEditing, false);
  assert.strictEqual(
    bankManagerControl.presentation.rows[0].instanceId,
    "instance.1",
  );
  assert.deepStrictEqual(
    bankManagerControl.presentation.rows[0].banks[0].color,
    [0.1, 0.2, 0.3, 0.4],
  );
  assert.strictEqual(
    bankManagerControl.presentation.rows[0].banks[0].textColor,
    null,
  );
  assert.deepStrictEqual(
    bankManagerControl.presentation.linkGroups[0].color,
    [0.5, 0.6, 0.7, 0.8],
  );
  assert.strictEqual(bankManagerControl.presentation.editAction.enabled, true);
  assert.strictEqual(bankManagerControl.presentation.clearAction.armed, true);
}
function testBankManagerForwardsShiftSelection() {
  var intents = [];
  var presentation = new BankManagerPresentation();
  presentation.rows = [{
    instanceId: "instance.1",
    label: "Local",
    local: true,
    banks: [{
      bankId: 1,
      label: "1",
      visible: true,
      enabled: true,
    }],
  }];
  bankManagerControl.applyPresentation(presentation);
  bankManagerControl.emit = function (name, values) {
    intents.push([name, values]);
  };

  onclick(105, 5, 1, 0, 1);
  ondrag(105, 5, 0);
  assert.deepStrictEqual(intents, [[
    "bankSelected",
    ["instance.1", 1, 1],
  ]]);
}
function testBankManagerPresentsGroupingSelectionAsActive() {
  vm.runInThisContext(
    fs.readFileSync(
      path.join(root, "js/Presenters/BankManager/BankManagerPresenter.js"),
      "utf8",
    ),
    { filename: "js/Presenters/BankManager/BankManagerPresenter.js" },
  );
  var viewModel = {
    enabled: true,
    linkEditing: true,
    rows: [{
      instanceId: "instance.1",
      label: "Local",
      local: true,
      banks: [{
        bankId: 1,
        active: false,
        linkSelected: true,
      }],
    }],
    linkGroups: [],
    editAction: { enabled: true, active: true },
    clearAction: { enabled: false, armed: false },
    subscribe: function () { return function () {}; },
  };
  var presenter = new BankManagerPresenter(viewModel);

  assert.strictEqual(presenter.presentation.rows[0].banks[0].active, true);
  presenter.destroy();
}
testControlBindingsDispatchByControlId();
testDialBindingUsesMessageTransportAndIntents();
testDialBindingWrapsGesturesInTransactions();
testDialBindingReportsRejectedTransactionWithoutWriting();
testButtonBindingPreservesPresentationMetadata();
testUiHostRoutesIntentsByControlVarname();
testPanelBindingHostRoutesListMessagesToNamedControl();
testAnalyzerHandlesPublishOnlyAfterTargetSnapshotIsReady();
testAnalyzerBindingUsesOneTransactionForHandleDrag();
testAnalyzerHandleDragPublishesLatestPositionWhileDragging();
testUiHostAcceptsTrackNameMessage();
testUiHostLoadsInIsolatedMaxContext();
testFeaturePresenterSetEnumeratesTypedPresenters();
testFeaturePresenterSetTracksSourceAvailability();
testDialDisplayScaleDoesNotChangePhysicalValue();
testConsolidatorInitializesDetectorState();
testFilterPositionUsesOneStateBatch();
testDetectorPositionUsesOneStateBatch();
testDetectorBypassIsInvertedForPresentation();
testMessageControlsConstructCompletePresentation();
testBankManagerForwardsShiftSelection();
testBankManagerPresentsGroupingSelectionAsActive();
console.log("UiBindingTests passed");
