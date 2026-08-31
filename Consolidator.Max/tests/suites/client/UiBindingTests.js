var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");
var createRequire = require("module").createRequire;
var environment = require("../../support/ProductionEnvironment.js");
var stateFixtures = require("../../support/StateFixtures.js");
var root = environment.root;
environment.loadClientEnvironment();
var makeStateFixture = stateFixtures.makeStateFixture;
var BankManagerController = require(
  path.join(root, "js/Controllers/BankManagerController.js")
).BankManagerController;

function loadMaxClass(relativePath, className) {
  var absolutePath = path.join(root, relativePath);
  var source = fs.readFileSync(absolutePath, "utf8");
  var factory = vm.runInThisContext(
    "(function (require, mgraphics, outlet, Task) {\n" +
      source + "\nreturn " + className + ";\n})",
    { filename: relativePath },
  );
  return factory(
    createRequire(absolutePath),
    global.mgraphics,
    global.outlet,
    global.Task,
  );
}

var BankManagerControl = loadMaxClass(
  "js/Controls/BankManager/BankManagerControl.js",
  "BankManagerControl",
);
var bankManagerControl = new BankManagerControl();
var DoubleClickTracker = require(
  path.join(root, "js/Controls/DoubleClickTracker.js")
).DoubleClickTracker;

function testDoubleClickTrackerRecognizesOnlyTheSameControl() {
  var tracker = new DoubleClickTracker();
  assert.strictEqual(tracker.isDoubleClick("a"), false);
  assert.strictEqual(tracker.isDoubleClick("a"), true);
  assert.strictEqual(tracker.isDoubleClick("b"), false);
}

function testControlBindingsDispatchByControlId() {
  var calls = [];
  var bindings = new ControlBindings();
  bindings.add("compressor_threshold", {
    handleIntent: function (intent, values) {
      calls.push([intent, values]);
    },
    setPresentationActive: function () {},
    destroy: function () {},
  });

  bindings.handle("compressor_threshold", "valueChanged", [0, 0.5]);
  bindings.handle("compressor_ratio", "valueChanged", [0, 4]);

  assert.deepStrictEqual(calls, [["valueChanged", [0, 0.5]]]);
  assert.throws(function () {
    bindings.add("compressor_threshold", {
      handleIntent: function () {},
      setPresentationActive: function () {},
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
    setValue: function (index, value, scope) {
      intents.push(["setValue", index, value, scope]);
    },
    resetValue: function (index, transactionId, scope) {
      intents.push(["resetValue", index, scope]);
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
    ["presentation_begin", []],
    ["enabled", [1]],
    ["active", [1]],
    ["activeIndex", [0]],
    ["displayIndex", [0]],
    ["ringCount", [1]],
    ["limits", [0, 0, 1]],
    ["set", [0, 0.5]],
    ["presentation_end", []],
  ]);
  binding.handleIntent("valueChanged", [0, 0.75, "local"]);
  binding.handleIntent("reset", [0, "group"]);
  binding.handleIntent("gestureBegan", [0, "group"]);
  binding.handleIntent("gestureEnded", [0, "group"]);
  assert.deepStrictEqual(intents, [
    ["setValue", 0, 0.75, "local"],
    ["resetValue", 0, "group"],
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

  binding.handleIntent("gestureBegan", [0, "local"]);
  binding.handleIntent("valueChanged", [0, 0.75, "local"]);
  binding.handleIntent("gestureEnded", [0, "local"]);
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
    ["presentation_begin", []],
    ["set", [1]],
    ["enabled", [1]],
    ["active", [1]],
    ["mode", ["momentary"]],
    ["label", ["SOLO"]],
    ["presentation_end", []],
  ]);
  binding.destroy();
}

function testDialBindingResumesOnlyTheLatestPresentation() {
  var messages = [];
  var listener = null;
  var presentation = {
    enabled: true,
    active: true,
    activeIndex: 0,
    displayIndex: 0,
    rings: [{ minimum: 0, maximum: 1, value: 0.25 }],
  };
  var presenter = {
    presentation: presentation,
    subscribe: function (callback, immediate) {
      listener = callback;
      if (immediate) callback(this.presentation);
      return function () {};
    },
  };
  var binding = new DialControlBinding(presenter, function (name, args) {
    messages.push([name, args]);
  });
  messages = [];

  binding.setPresentationActive(false);
  presentation = {
    enabled: true,
    active: true,
    activeIndex: 0,
    displayIndex: 0,
    rings: [{ minimum: 0, maximum: 1, value: 0.5 }],
  };
  presenter.presentation = presentation;
  listener(presentation);
  presentation = {
    enabled: true,
    active: true,
    activeIndex: 0,
    displayIndex: 0,
    rings: [{ minimum: 0, maximum: 1, value: 0.75 }],
  };
  presenter.presentation = presentation;
  listener(presentation);

  assert.deepStrictEqual(messages, []);
  binding.setPresentationActive(true);
  assert.deepStrictEqual(messages.filter(function (message) {
    return message[0] === "set";
  }), [["set", [0, 0.75]]]);
  binding.destroy();
}
function testBankManagerBindingPatchesRegistryAddition() {
  var messages = [];
  var listener = null;
  var initial = {
    enabled: true,
    rows: [{
    instanceId: "1",
    label: "First",
    local: true,
    processors: [{
      processorId: "equalizer", effectActive: false,
      markerActive: false, bypassed: false, soloed: false,
    }],
    banks: [{ bankId: 1, label: "1", visible: true, enabled: true }],
    }],
    clearAction: null,
    delta: null,
  };
  var presenter = {
    subscribe: function (callback, immediate) {
      listener = callback;
      if (immediate) callback(initial);
      return function () {};
    },
  };
  var binding = new BankManagerControlBinding(
    { handleIntent: function () {} },
    presenter,
    function (name, args) { messages.push([name, args]); },
  );
  messages = [];

  var updated = {
    enabled: true,
    rows: initial.rows.concat([{
      instanceId: "2",
      label: "Second",
      local: false,
      processors: [{
        processorId: "equalizer", effectActive: true,
        markerActive: true, bypassed: false, soloed: false,
      }],
      banks: [{ bankId: 1, label: "1", visible: true, enabled: true }],
    }]),
    clearAction: null,
    delta: {
      selector: "registry_instance_added",
      args: [1, 0, 1, "2"],
      rowIndex: 1,
    },
  };
  listener(updated);

  assert.strictEqual(messages[0][0], "presentation_patch_begin");
  assert.strictEqual(messages[messages.length - 1][0],
    "presentation_patch_end");
  assert.strictEqual(messages.filter(function (message) {
    return message[0] === "presentation_begin";
  }).length, 0);
  assert.deepStrictEqual(messages.filter(function (message) {
    return message[0] === "row_patch";
  })[0][1].slice(0, 3), [1, "2", "Second"]);

  messages = [];
  updated.rows[0].banks[0].active = false;
  updated.rows[1].banks[0].active = true;
  updated.rows[0].processors[0].markerActive = false;
  updated.rows[1].processors[0].markerActive = true;
  updated.delta = {
    selector: "bank_focus_changed",
    previousRowIndex: 0,
    previousBankId: 1,
    rowIndex: 1,
    bankId: 1,
  };
  listener(updated);
  assert.strictEqual(messages.filter(function (message) {
    return message[0] === "bank_patch";
  }).length, 2);
  assert.deepStrictEqual(messages.filter(function (message) {
    return message[0] === "processor_patch";
  }), []);
  assert.strictEqual(messages.filter(function (message) {
    return message[0] === "row_patch" ||
      message[0] === "presentation_begin";
  }).length, 0);

  messages = [];
  updated.delta = {
    selector: "registry_bank_group_changed",
    args: [1, 1, 2, "1", 1, 4],
    rowIndex: 0,
  };
  listener(updated);
  assert.deepStrictEqual(messages.filter(function (message) {
    return message[0] === "processor_patch";
  }), []);

  messages = [];
  updated.rows[0].processors[0].markerActive = true;
  updated.delta = {
    selector: "registry_processor_markers_changed",
    args: [1, 1, "1", 1, "equalizer", 1],
    rowIndices: [0],
  };
  listener(updated);
  assert.deepStrictEqual(messages.filter(function (message) {
    return message[0] === "processor_patch";
  }).map(function (message) {
    return message[1].slice(0, 4);
  }), [[0, "equalizer", 0, 1]]);

  messages = [];
  binding.setPresentationActive(false);
  updated.delta = {
    selector: "registry_label_changed",
    rowIndex: 1,
  };
  updated.rows[1].label = "Latest";
  listener(updated);
  assert.deepStrictEqual(messages, []);

  binding.setPresentationActive(true);
  assert.strictEqual(messages[0][0], "presentation_begin");
  assert.strictEqual(messages[messages.length - 1][0], "presentation_end");
  assert.strictEqual(messages.filter(function (message) {
    return message[0] === "presentation_patch_begin";
  }).length, 0);
  assert.deepStrictEqual(messages.filter(function (message) {
    return message[0] === "row" && message[1][0] === 1;
  })[0][1].slice(0, 3), [1, "2", "Latest"]);
  binding.destroy();
}
function testUiHostRoutesIntentsByControlVarname() {
  var calls = [];
  var nativeMessages = [];
  var host = Object.create(ConsolidatorUiHost.prototype);
  host.bindings = new ControlBindings();
  host.sendNative = function (message) { nativeMessages.push(message); };
  host.metricsGestureActive = false;
  host.sendControlMessage = function () {};
  host.bind("input_gain", function () {
    return {
      handleIntent: function (name, values) {
        calls.push([name, values]);
      },
      setPresentationActive: function () {},
      destroy: function () {},
    };
  });

  host.handleUiIntent("input_gain", "gestureBegan", [0]);
  host.handleUiIntent("input_gain", "valueChanged", [0, 0.75]);
  host.handleUiIntent("input_gain", "gestureEnded", [0]);
  assert.deepStrictEqual(calls, [
    ["gestureBegan", [0]],
    ["valueChanged", [0, 0.75]],
    ["gestureEnded", [0]],
  ]);
  assert.deepStrictEqual(nativeMessages, [["metrics"], ["metrics"]]);
  host.bindings.destroy();
}
function testPanelBindingHostRoutesListMessagesToNamedControl() {
  var calls = [];
  var lookups = 0;
  var context = vm.createContext({});
  vm.runInContext(
    fs.readFileSync(path.join(root, "js/PanelBindingHostV8.js"), "utf8"),
    context,
    { filename: "js/PanelBindingHostV8.js" },
  );
  context.patcher = {
    getnamed: function (name) {
      lookups += 1;
      assert.strictEqual(name, "input_gain");
      return {
        message: function () {
          calls.push(Array.prototype.slice.call(arguments));
        },
      };
    },
  };

  context.list(0, "input_gain", "ringCount", 1);
  context.list(0, "input_gain", "set", 0, 0.5);

  assert.deepStrictEqual(calls, [
    ["ringCount", 1],
    ["set", 0, 0.5],
  ]);
  assert.strictEqual(lookups, 1);
}
function testUiHostEntrypointInitializesFromLiveReadyAndRoutesLists() {
  var initializedWith = null;
  var controls = [];
  var instance = null;
  var mapping = { bankManager: "bank_manager" };

  function FakeUiHost() {
    instance = this;
  }
  FakeUiHost.prototype.initialize = function (value) {
    initializedWith = value;
  };
  FakeUiHost.prototype.handleControl = function (selector, args) {
    controls.push([selector, Array.prototype.slice.call(args)]);
  };
  FakeUiHost.prototype.setTrackName = function () {};
  FakeUiHost.prototype.handleUiIntent = function () {};
  FakeUiHost.prototype.undo = function () {};
  FakeUiHost.prototype.redo = function () {};
  FakeUiHost.prototype.destroy = function () {};

  var context = vm.createContext({
    jsarguments: ["ConsolidatorUiHost.js", "bridge.local"],
    outlet: function () {},
    require: function (request) {
      assert.strictEqual(request, "./ConsolidatorUiApplication.js");
      return {
        ConsolidatorUiHost: FakeUiHost,
        ConsolidatorControlMapping: mapping,
      };
    },
  });
  vm.runInContext(
    fs.readFileSync(path.join(root, "js/ConsolidatorUiHost.js"), "utf8"),
    context,
    { filename: "js/ConsolidatorUiHost.js" },
  );

  context.live_ready();
  assert.strictEqual(initializedWith, mapping);
  assert.ok(instance);
  context.list.call({ inlet: 0 }, 0, "instance_active", 1);
  assert.deepStrictEqual(controls, [["instance_active", [1]]]);
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
  assert.strictEqual(presenter.presentation.handles.length, 1);
  presenter.filterMoved(1, 0.5, 0.25, 7);
  assert.strictEqual(writes.length, 1);
  assert.strictEqual(writes[0][2], 7);
  presenter.destroy();
}

function testAnalyzerPublishesOneSpectrumNotificationPerFftFrame() {
  var protocolHandlers = {};
  var notifications = 0;
  var protocol = {
    on: function (selector, callback) {
      protocolHandlers[selector] = callback;
      return function () {};
    },
  };
  var presenter = new AnalyzerPresenter({
    statusSource: {
      subscribeStatus: function (callback, immediate) {
        if (immediate) callback({
          ready: true,
          target: { instanceId: 7, bankId: 1 },
        });
        return function () {};
      },
    },
  });
  presenter.connectSpectrum(protocol);
  presenter.subscribeSpectrum(function () {
    notifications += 1;
  });

  protocolHandlers.fft([
    1, 8, 4,
    0.1, 0.2, 0.3,
    0.4, 0.5, 0.6,
  ]);
  protocolHandlers.fft([
    1, 7, 4,
    0.1, 0.2, 0.3,
    0.4, 0.5, 0.6,
  ]);

  assert.strictEqual(notifications, 1);
  assert.strictEqual(presenter.spectrum.values.length, 3);
  assert.strictEqual(presenter.referenceSpectrum.values.length, 3);
  presenter.destroy();
}
function testAnalyzerDragClampsToEffectivePeerRanges() {
  var writes = [];
  var presenter = new AnalyzerPresenter({
    parameters: [{
      frequency: {
        value: 1000,
        minimum: 500,
        maximum: 2000,
      },
      gain: {
        value: 0,
        minimum: -3,
        maximum: 6,
      },
      setPosition: function (frequency, gain) {
        writes.push([frequency, gain]);
      },
    }],
  });

  presenter.filterMoved(1, 0, 0);
  presenter.filterMoved(1, 1, 1);

  assert.deepStrictEqual(writes, [
    [500, 6],
    [2000, -3],
  ]);
  presenter.destroy();
}
function testAnalyzerParameterUpdatesRecalculateLocalCurves() {
  var parameterListeners = [];
  var frequency = {
    value: 1000,
    subscribe: function (callback) {
      parameterListeners.push(callback);
      return function () {};
    },
  };
  var presenter = new AnalyzerPresenter({
    parameters: [{
      frequency: frequency,
      gain: { value: 6 },
      q: { value: 1 },
      enabled: true,
    }],
  });
  var previous = presenter.curves[0].values.slice(0);

  frequency.value = 1500;
  parameterListeners[0]();

  assert.notDeepStrictEqual(presenter.curves[0].values, previous);
  presenter.destroy();
}
function testAnalyzerGestureKeepsPreviewUntilFinalWriteCompletes() {
  var parameterListeners = [];
  var frequency = {
    value: 1000,
    subscribe: function (callback) {
      parameterListeners.push(callback);
      return function () {};
    },
  };
  var presenter = new AnalyzerPresenter({
    parameters: [{
      frequency: frequency,
      gain: { value: 0 },
      q: { value: 1 },
      enabled: true,
    }],
  });

  presenter.beginPreviewGesture();
  presenter.previewMoved(1, 0.75, 0.25);
  var previewCurve = presenter.curves[0].values.slice(0);
  frequency.value = 1200;
  parameterListeners[0]();

  assert.ok(presenter.curvePreview[1]);
  assert.deepStrictEqual(presenter.curves[0].values, previewCurve);
  presenter.endPreviewGesture();
  assert.strictEqual(presenter.curvePreview[1], undefined);
  assert.notDeepStrictEqual(presenter.curves[0].values, previewCurve);
  presenter.destroy();
}
function testAnalyzerConfigurationRecalculatesCurvesForSampleRate() {
  var protocolHandlers = {};
  var presenter = new AnalyzerPresenter({
    statusSource: {
      subscribeStatus: function (callback, immediate) {
        if (immediate) callback({
          ready: true,
          target: { instanceId: 7, bankId: 1 },
        });
        return function () {};
      },
    },
    parameters: [{
      frequency: { value: 18000 },
      gain: { value: 12 },
      q: { value: 1 },
    }],
  });
  presenter.connectConfiguration({
    on: function (selector, callback) {
      protocolHandlers[selector] = callback;
      return function () {};
    },
  });
  var previous = presenter.curves[0].values.slice(0);

  protocolHandlers.analyzer_configuration([1, 8, 32000]);
  assert.strictEqual(presenter.sampleRate, 48000);
  protocolHandlers.analyzer_configuration([1, 7, 44100]);

  assert.strictEqual(presenter.sampleRate, 44100);
  assert.notDeepStrictEqual(presenter.curves[0].values, previous);
  presenter.destroy();
}
function testEqualizerPresenterBuildsAllBanksCurveFromRawState() {
  var protocolHandlers = {};
  var presenter = new AnalyzerPresenter({
    mode: "equalizer",
    statusSource: {
      subscribeStatus: function (callback, immediate) {
        if (immediate) callback({
          ready: true,
          target: { instanceId: 7, bankId: 0 },
        });
        return function () {};
      },
    },
    parameters: [{
      frequency: { value: 1000 },
      gain: { value: 6 },
      q: { value: 1 },
      enabled: true,
    }],
  });
  presenter.connectConfiguration({
    on: function (selector, callback) {
      protocolHandlers[selector] = callback;
      return function () {};
    },
  });

  protocolHandlers.analyzer_equalizer_state([
    1, 2, 8, 2, 1,
    1, 2,
    1, "bell", 0.707, 3, "frequency", 1000, "q", 1, "gain", 6,
    1, "bell", 0.707, 3, "frequency", 2000, "q", 1, "gain", 6,
  ]);
  assert.strictEqual(presenter.allBanksCurve.active, false);
  assert.strictEqual(presenter.equalizerState.sourceInstanceId, "8");

  protocolHandlers.analyzer_equalizer_state([
    1, 2, 7, 2, 1,
    1, 2,
    1, "bell", 0.707, 3, "frequency", 1000, "q", 1, "gain", 6,
    1, "bell", 0.707, 3, "frequency", 2000, "q", 1, "gain", 6,
  ]);

  assert.strictEqual(presenter.allBanksCurve.active, true);
  assert.strictEqual(presenter.equalizerState.sourceInstanceId, "7");
  assert.strictEqual(presenter.allBanksCurve.values.length, 256);
  assert.notDeepStrictEqual(
    presenter.allBanksCurve.values,
    presenter.combinedCurve.values,
  );
  presenter.destroy();
}
function testDetectorPresenterBuildsFilterCurves() {
  var presenter = new AnalyzerPresenter({
    mode: "detector",
    parameters: [
      { frequency: { value: 500 }, gain: { value: 3 }, q: { value: 1 } },
      { frequency: { value: 2000 }, gain: { value: -3 }, q: { value: 1 } },
    ],
  });

  assert.strictEqual(presenter.curves.length, 2);
  assert.strictEqual(presenter.curves[0].values.length, 256);
  assert.strictEqual(presenter.curves[1].values.length, 256);
  assert.strictEqual(presenter.combinedCurve.values.length, 256);
  presenter.destroy();
}
function testDetectorPresenterUpdatesCurvesDuringFilterDrag() {
  var presenter = new AnalyzerPresenter({
    mode: "detector",
    parameters: [{
      frequency: { value: 1000 },
      gain: { value: 6 },
      q: { value: 1 },
    }],
  });
  var previous = presenter.curves[0].values.slice(0);
  presenter.previewMoved(1, 0.75, 0.25);

  assert.notDeepStrictEqual(presenter.curves[0].values, previous);
  presenter.destroy();
}
function testAnalyzerRendersFixedQFilterWithoutEditableQ() {
  var presenter = new AnalyzerPresenter({
    mode: "equalizer",
    parameters: [{
      definition: {
        type: "low_shelf",
        fixedQ: 0.707,
        parameters: { frequency: {}, gain: {} },
      },
      frequency: { value: 100 },
      q: null,
      gain: { value: 6 },
      enabled: true,
    }],
  });

  assert.ok(presenter.curves[0].values.some(function (value) {
    return value < 0.49;
  }));
  presenter.destroy();
}
function testAnalyzerRendersLegacyHighShelfAndTiltResponses() {
  ["high_shelf", "tilt"].forEach(function (type) {
    var presenter = new AnalyzerPresenter({
      mode: "equalizer",
      sampleRate: 44100,
      parameters: [{
        definition: {
          type: type,
          fixedQ: 0.707,
          parameters: { frequency: {}, gain: {} },
        },
        frequency: { value: type === "tilt" ? 1000 : 10000 },
        q: null,
        gain: { value: 6 },
        enabled: true,
      }],
    });
    var values = presenter.curves[0].values;
    assert.ok(values.some(function (value) { return value < 0.49; }));
    if (type === "tilt") {
      assert.ok(values.some(function (value) { return value > 0.51; }));
    }
    presenter.destroy();
  });
}
function testDetectorBindingPublishesCurvesToControl() {
  var messages = [];
  var presenter = new AnalyzerPresenter({
    mode: "detector",
    parameters: [
      { frequency: { value: 500 }, gain: { value: 3 }, q: { value: 1 } },
      { frequency: { value: 2000 }, gain: { value: -3 }, q: { value: 1 } },
    ],
  });
  var binding = new AnalyzerControlBinding(
    {},
    presenter,
    function (name, args) {
      messages.push([name, args]);
    }
  );
  var curves = messages.filter(function (message) {
    return message[0] === "curve";
  });
  var combined = messages.filter(function (message) {
    return message[0] === "combined";
  });

  assert.strictEqual(curves.length, 2);
  assert.strictEqual(curves[0][1][0], 1);
  assert.strictEqual(curves[0][1].length, 258);
  assert.strictEqual(combined.length, 1);
  assert.strictEqual(combined[0][1].length, 256 + 1);

  messages = [];
  binding.setPresentationActive(false);
  presenter.previewMoved(1, 0.75, 0.25);
  assert.deepStrictEqual(messages, []);

  binding.setPresentationActive(true);
  assert.strictEqual(messages.filter(function (message) {
    return message[0] === "curve";
  }).length, 2);
  assert.strictEqual(messages.filter(function (message) {
    return message[0] === "combined";
  }).length, 1);
  binding.destroy();
  presenter.destroy();
}
function testAnalyzerControlPreservesCurvesAcrossHandlePresentation() {
  var previousTask = global.Task;
  global.Task = function () {
    this.schedule = function () {};
    this.cancel = function () {};
  };
  var AnalyzerControl = loadMaxClass(
    "js/Controls/Analyzer/AnalyzerControl.js",
    "AnalyzerControl",
  );
  var analyzerControl = new AnalyzerControl();
  analyzerControl.beginPresentation("detector", 1, 0, "");
  analyzerControl.addCurve("curve", [1, 0.5, 0.5], 1);
  analyzerControl.addCurve("combined", [1, 0.5, 0.5]);
  analyzerControl.addCurve("all_banks", [1, 0.4, 0.4]);
  analyzerControl.applyPresentation(analyzerControl.pendingPresentation);
  analyzerControl.pendingPresentation = null;

  assert.strictEqual(analyzerControl.presentation.curves.length, 1);
  assert.strictEqual(
    analyzerControl.presentation.combinedCurve.values.length,
    2
  );
  assert.strictEqual(
    analyzerControl.presentation.allBanksCurve.values.length,
    2
  );

  analyzerControl.beginPresentation("detector", 1, 0, "");
  analyzerControl.applyPresentation(analyzerControl.pendingPresentation);
  analyzerControl.pendingPresentation = null;
  assert.strictEqual(analyzerControl.presentation.curves.length, 1);
  assert.strictEqual(
    analyzerControl.presentation.combinedCurve.values.length,
    2
  );
  assert.strictEqual(
    analyzerControl.presentation.allBanksCurve.values.length,
    2
  );
  analyzerControl.destroy();
  global.Task = previousTask;
}
function testAnalyzerBindingUsesOneTransactionForHandleDrag() {
  var calls = [];
  var transactionCalls = [];
  var beginCallback = null;
  var commitCallback = null;
  var controller = {
    handle: function (intent, payload, transactionId, callback) {
      calls.push([intent, payload, transactionId]);
      if (intent === "filterCommit") {
        commitCallback = callback;
      }
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

  binding.handleIntent("gestureBegan", [1, "group"]);
  binding.handleIntent("filterMoved", [1, 0.4, 0.6, "group"]);
  binding.handleIntent("filterMoved", [1, 0.5, 0.7, "group"]);
  binding.handleIntent("gestureEnded", [1, "group"]);
  beginCallback(12, { status: "accepted" });

  assert.deepStrictEqual(calls, [
    ["gestureBegan", [1], null],
    ["filterMoved", [1, 0.4, 0.6, "group"], 12],
    ["filterMoved", [1, 0.5, 0.7, "group"], 12],
    ["filterCommit", [1, "group"], 12],
    ["gestureEnded", [], 12],
  ]);

  assert.deepStrictEqual(transactionCalls, [
    ["begin"],
    ["end", 12],
  ]);
  assert.deepStrictEqual(calls, [
    ["gestureBegan", [1], null],
    ["filterMoved", [1, 0.4, 0.6, "group"], 12],
    ["filterMoved", [1, 0.5, 0.7, "group"], 12],
    ["filterCommit", [1, "group"], 12],
    ["gestureEnded", [], 12],
  ]);
  commitCallback({ status: "accepted", error: null });
  binding.destroy();
}

function testAnalyzerStartsNextHistoryPointBeforePreviousWriteResponse() {
  var beginCallbacks = [];
  var commitCallbacks = [];
  var beginIds = [12, 13];
  var controller = {
    handle: function (intent, payload, transactionId, callback) {
      if (intent === "filterCommit") {
        commitCallbacks.push(callback);
      }
    },
  };
  var presenter = {
    subscribe: function () {
      return function () {};
    },
  };
  var transactions = {
    begin: function (callback) {
      var id = beginIds[beginCallbacks.length];
      beginCallbacks.push(callback);
      return id;
    },
    end: function () {},
  };
  var binding = new AnalyzerControlBinding(
    controller,
    presenter,
    function () {},
    transactions
  );

  binding.handleIntent("gestureBegan", [1]);
  beginCallbacks[0](12, { status: "accepted" });
  binding.handleIntent("filterMoved", [1, 0.4, 0.6]);
  binding.handleIntent("gestureEnded", [1]);
  assert.strictEqual(binding.activeTransactionId, null);

  binding.handleIntent("gestureBegan", [1]);
  assert.strictEqual(beginCallbacks.length, 2);
  assert.strictEqual(binding.activeTransactionId, 13);

  commitCallbacks[0]({ status: "accepted", error: null });
  binding.destroy();
}

function testAnalyzerTargetTransitionCancelsGestureAndTransaction() {
  var calls = [];
  var transactionCalls = [];
  var messages = [];
  var beginCallback = null;
  var beginCount = 0;
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
      beginCount += 1;
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
    function (selector) { messages.push(selector); },
    transactions
  );

  binding.handleIntent("gestureBegan", [1]);
  beginCallback(12, { status: "accepted" });
  binding.handleIntent("filterMoved", [1, 0.4, 0.6]);
  binding.suspend();

  assert.deepStrictEqual(transactionCalls, [["end", 12]]);
  assert.deepStrictEqual(calls[calls.length - 1], ["gestureEnded", [], 12]);
  assert.strictEqual(binding.activeTransactionId, null);
  assert.strictEqual(binding.transactionReady, false);
  assert.strictEqual(binding.lastMove, null);
  assert.strictEqual(messages[messages.length - 1], "interactionReset");

  binding.handleIntent("gestureBegan", [1]);
  assert.strictEqual(beginCount, 2);

  binding.destroy();
}
function testAnalyzerHandleDragPublishesLatestPositionWhileDragging() {
  var messages = [];
  var scheduled = null;
  var scheduledDelays = [];
  var previousTask = global.Task;
  global.Task = function (callback) {
    this.schedule = function (delay) {
      scheduledDelays.push(delay);
      if (scheduled === null) scheduled = callback;
    };
    this.cancel = function () {
      scheduled = null;
    };
  };
  global.mgraphics.size = [100, 100];
  var AnalyzerControl = loadMaxClass(
    "js/Controls/Analyzer/AnalyzerControl.js",
    "AnalyzerControl",
  );
  var analyzerControl = new AnalyzerControl();
  analyzerControl.applyPresentation({
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
      xMinimum: 0.2,
      xMaximum: 0.6,
      yMinimum: 0.3,
      yMaximum: 0.8,
    }],
  });
  assert.deepStrictEqual(scheduledDelays, [16]);
  scheduled();
  scheduled = null;

  analyzerControl.state.selectedId = analyzerControl.hitTest(52, 50);
  analyzerControl.state.dragging = true;
  analyzerControl.emitIntent = function (name, values) {
    messages.push([name].concat(values || []));
  };
  analyzerControl.emitIntent("filterSelected", [1]);
  analyzerControl.emitIntent("gestureBegan", [1]);
  analyzerControl.scheduleMove(1, 0.6, 0.3);
  analyzerControl.scheduleMove(1, 0.6, 0.3);
  assert.deepStrictEqual(scheduledDelays, [16, 33]);
  assert.deepStrictEqual(messages, [
    ["filterSelected", 1],
    ["gestureBegan", 1],
  ]);

  scheduled();
  scheduled = null;
  assert.strictEqual(messages.length, 3);
  assert.strictEqual(messages[2][0], "filterMoved");
  assert.strictEqual(messages[2][1], 1);
  assert.strictEqual(messages[2][2], 0.6);
  assert.strictEqual(messages[2][3], 0.3);

  analyzerControl.endGesture();
  assert.deepStrictEqual(messages[3], ["gestureEnded", 1]);
  global.Task = previousTask;
  analyzerControl.destroy();
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
  assert.deepStrictEqual(state.sets, [["label", "Drums"]]);

  ConsolidatorUiHost.prototype.setTrackName.call(host, []);
  assert.strictEqual(host.trackName, "");
  assert.deepStrictEqual(state.sets[1], ["label", ""]);
}
function testUiApplicationLoadsAsCommonJsV8Module() {
  var hostModule = require(path.join(root, "js/ConsolidatorUiApplication.js"));
  var host = new hostModule.ConsolidatorUiHost(
    "test.ui",
    function () {},
    function () {},
  );
  assert.strictEqual(host.lifecycle, "created");
  host.mapping = hostModule.ConsolidatorControlMapping;
  host.bindControls();
  assert.deepStrictEqual(
    Object.keys(host.bindings.items).sort(),
    Object.keys(hostModule.ConsolidatorControlMapping)
      .map(function (key) {
        return hostModule.ConsolidatorControlMapping[key];
      })
      .sort(),
  );
  host.destroy();
  assert.strictEqual(host.lifecycle, "destroyed");
  var recreated = new hostModule.ConsolidatorUiHost(
    "test.ui",
    function () {},
    function () {},
  );
  assert.strictEqual(recreated.lifecycle, "created");
  recreated.destroy();
  assert.strictEqual(recreated.lifecycle, "destroyed");
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
function testPresentationObservableBatchesOneRebuildAndStopsAfterDestroy() {
  var rebuilds = 0;
  var presenter = new PresentationObservable();
  presenter.rebuild = function () {
    rebuilds += 1;
  };

  PresentationObservable.beginBatch();
  presenter.requestRebuild();
  presenter.requestRebuild();
  PresentationObservable.endBatch();

  assert.strictEqual(rebuilds, 1);
  presenter.destroy();
  presenter.requestRebuild();
  assert.strictEqual(rebuilds, 1);
  presenter.destroy();
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
function testEqualizerPositionForwardsFinalWriteCallback() {
  var received = null;
  var callback = function () {};
  var parameters = EqualizerController.prototype.createBankParameters.call({}, [{
    frequency: {},
    gain: {},
    q: {},
    bypass: null,
    setPosition: function () {
      received = Array.prototype.slice.call(arguments);
    },
  }]);

  parameters[0].setPosition(750, 3, 12, callback);

  assert.deepStrictEqual(received, [750, 3, 12, callback]);
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
  var DialControl = loadMaxClass(
    "js/Controls/Dial/DialControl.js",
    "DialControl",
  );
  var dialControl = new DialControl();
  dialControl.setRingCount(1);
  dialControl.setPresentationLimits(0, 0.1, 0.9);
  dialControl.setPresentationValue(0, 0.4);
  assert.strictEqual(dialControl.presentation.rings.length, 1);
  assert.deepStrictEqual(dialControl.presentation.rings[0], {
    value: 0.4,
    minimum: 0.1,
    maximum: 0.9,
    visualization: null,
    color: null,
  });
  dialControl.dragging = true;
  dialControl.dragIndex = 0;
  dialControl.previewValues[0] = 0.8;
  dialControl.setPresentationValue(0, 0.5);
  assert.strictEqual(dialControl.presentation.rings[0].value, 0.5);
  assert.strictEqual(dialControl.previewValues[0], 0.8);
  dialControl.rejectTransaction();
  assert.strictEqual(dialControl.dragging, false);
  assert.deepStrictEqual(dialControl.previewValues, []);

  var ButtonControl = loadMaxClass(
    "js/Controls/Button/ButtonControl.js",
    "ButtonControl",
  );
  var buttonControl = new ButtonControl();
  buttonControl.setPresentationActive(1);
  buttonControl.setPresentationMode("momentary");
  buttonControl.setPresentationLabel("SOLO");
  assert.strictEqual(buttonControl.presentation.active, true);
  assert.strictEqual(buttonControl.presentation.mode, "momentary");
  assert.strictEqual(buttonControl.presentation.label, "SOLO");

  var BankManagerControl = loadMaxClass(
    "js/Controls/BankManager/BankManagerControl.js",
    "BankManagerControl",
  );
  var bankManagerControl = new BankManagerControl();
  bankManagerControl.beginPresentation(1);
  bankManagerControl.addRow(0, "instance.1", "Local", 1);
  bankManagerControl.addProcessor(0, "equalizer", 1, 0, 0, 0);
  bankManagerControl.addBank(0, 1, "1", 1, 1, 1, 1, 0, 0.75, 1, 1, 1, 0.1, 0.2, 0.3, 0.4, 0, 0, 0, 0, 0);
  bankManagerControl.setGroupAction(1, 0);
  bankManagerControl.setUngroupAction(0, 0);
  bankManagerControl.setClearAction(1, 1);
  bankManagerControl.endPresentation();

  assert.strictEqual(bankManagerControl.pendingPresentation, null);
  assert.strictEqual(bankManagerControl.presentation.enabled, true);
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
  assert.strictEqual(bankManagerControl.presentation.groupAction.enabled, true);
  assert.strictEqual(bankManagerControl.presentation.clearAction.armed, true);
  assert.strictEqual(
    bankManagerControl.presentation.rows[0].processors[0].markerActive,
    false,
  );
  assert.strictEqual(
    bankManagerControl.presentation.rows[0].processors[0].effectActive,
    true,
  );

  bankManagerControl.beginPresentationPatch(1);
  bankManagerControl.patchRow(0, "instance.1", "Renamed", 1);
  bankManagerControl.patchProcessor(0, "equalizer", 1, 0, 0, 0);
  bankManagerControl.patchBank(0, 1, "1", 0, 1, 1, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  bankManagerControl.endPresentationPatch();
  assert.strictEqual(
    bankManagerControl.presentation.rows[0].label,
    "Renamed",
  );
  assert.strictEqual(
    bankManagerControl.presentation.rows[0].banks[0].active,
    false,
  );
  assert.strictEqual(
    bankManagerControl.presentation.rows[0].processors[0].markerActive,
    false,
  );
}
function testUiHostPublishesOnlyChangedInstanceActivity() {
  var values = [];
  var host = {
    client: {
      setInstanceActive: function (active) { values.push(active); },
    },
    bindings: {
      setPresentationActive: function () {},
    },
    bankManagerViewModel: {
      setRegistryActive: function () {},
    },
    instanceActive: false,
    publishedInstanceActive: null,
  };

  ConsolidatorUiHost.prototype.setInstanceActive.call(host, true);
  assert.deepStrictEqual(values, []);
  host.instanceId = "7";
  ConsolidatorUiHost.prototype.setInstanceActive.call(host, true);
  ConsolidatorUiHost.prototype.setInstanceActive.call(host, true);
  ConsolidatorUiHost.prototype.setInstanceActive.call(host, false);

  assert.deepStrictEqual(values, [true, false]);
}
function testUiHostRefreshesSnapshotBeforePresentingActivatedInstance() {
  var events = [];
  var host = {
    client: {
      targetState: {
        target: { instanceId: "9", bankId: 3 },
      },
      setInstanceActive: function (active, callback) {
        events.push(["managed", active]);
        callback({ status: "accepted", error: null });
      },
      uiTarget: {
        show: function (instanceId, bankId, snapshotContext, callback) {
          events.push(["snapshot", instanceId, bankId]);
          callback({ entries: [], snapshotContext: snapshotContext, error: null });
        },
      },
    },
    bindings: {
      setPresentationActive: function (active) {
        events.push(["bindings", active]);
      },
    },
    bankManagerViewModel: {
      setRegistryActive: function (active) {
        events.push(["registry", active]);
      },
    },
    instanceId: "7",
    instanceActive: false,
    publishedInstanceActive: null,
  };

  ConsolidatorUiHost.prototype.setInstanceActive.call(host, true);
  ConsolidatorUiHost.prototype.setInstanceActive.call(host, true);

  assert.deepStrictEqual(events, [
    ["registry", true],
    ["bindings", false],
    ["managed", true],
    ["snapshot", "9", 3],
    ["bindings", true],
  ]);
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

  global.mgraphics.size = [800, 400];
  bankManagerControl.selectAt(
    bankManagerControl.bankGridX(presentation.rows) + 12,
    5,
    true,
  );
  assert.deepStrictEqual(intents, [[
    "bankSelected",
    ["instance.1", 1, 1],
  ]]);
}

function testBankManagerPanelClickWaitsForSnapshot() {
  var control = new BankManagerControl();
  var emitted = [];
  control.presentation = { enabled: true, selectedPanel: "equalizer" };
  control.panelAt = function () { return "compressor"; };
  control.emit = function (name, values) {
    emitted.push([name].concat(values || []));
  };

  control.selectAt(10, 10, false, false);

  assert.strictEqual(control.presentation.selectedPanel, "equalizer");
  assert.deepStrictEqual(emitted, [["panelSelected", "compressor"]]);
}

function testPanelTransitionAppliesSelectionAfterSnapshot() {
  var snapshotCallback = null;
  var context = {
    instanceId: "instance.1",
    viewModel: {
      selectedPanel: "equalizer",
      setSelectedPanel: function (panel) { this.selectedPanel = panel; },
    },
    uiTarget: {
      targetState: { target: { instanceId: "7", bankId: 2 } },
      show: function (instanceId, bankId, panel, callback) {
        assert.deepStrictEqual([instanceId, bankId, panel], ["7", 2, "compressor"]);
        snapshotCallback = callback;
      },
    },
    onSnapshotAccepted: function () {},
  };
  var controller = new BankManagerController(context);

  controller.handleIntent("panelSelected", ["compressor"]);
  assert.strictEqual(context.viewModel.selectedPanel, "equalizer");
  snapshotCallback({ snapshotContext: "compressor", error: null });
  assert.strictEqual(context.viewModel.selectedPanel, "compressor");
}
function testBankManagerForwardsInstanceControlModifiers() {
  var intents = [];
  var presentation = new BankManagerPresentation();
  presentation.rows = [{
    instanceId: "instance.1",
    label: "Local",
    local: true,
    solo: false,
    mute: false,
    banks: [{ bankId: 1, label: "1", visible: true, enabled: true }],
  }];
  bankManagerControl.applyPresentation(presentation);
  bankManagerControl.emit = function (name, values) {
    intents.push([name, values]);
  };

  global.mgraphics.size = [800, 400];
  var instanceButtonsX = bankManagerControl.bankGridRight(presentation.rows) +
    4 + 4;
  bankManagerControl.selectAt(instanceButtonsX + 8, 5, true, true);
  bankManagerControl.selectAt(instanceButtonsX + 24, 5, true, true);
  assert.deepStrictEqual(intents, [
    ["instanceSoloChanged", [1, 1, 1]],
    ["instanceMuteChanged", [1, 1]],
  ]);
}
function testBankManagerEqualizerResetReachesStateClient() {
  var resetCalls = [];
  var presentation = new BankManagerPresentation();
  presentation.rows = [{
    instanceId: "instance.1",
    label: "Local",
    local: true,
    processors: [{
      processorId: "equalizer",
      bypassed: false,
      soloed: false,
    }],
    banks: [{ bankId: 2, label: "2", visible: true, enabled: true }],
  }];
  presentation.enabled = true;
  bankManagerControl.applyPresentation(presentation);

  var context = {
    instanceId: "instance.1",
    viewModel: {
      focusedBank: function () { return { bankId: 2 }; },
    },
    state: {
      reset: function () {
        resetCalls.push(Array.prototype.slice.call(arguments));
      },
    },
  };
  var controller = new BankManagerController(context);
  bankManagerControl.emit = function (name, values) {
    controller.handleIntent(name, values);
  };
  bankManagerControl.flashReset = function () {};

  var x = bankManagerControl.primaryWidth(presentation.rows) +
    6 + 32 + 16 + 1 + 4;
  var y = 3 * 32 + 16 + 1 + 4;
  bankManagerControl.selectAt(x, y, false, true);

  assert.deepStrictEqual(resetCalls, [[
    "equalizer", undefined, 0, "group",
  ]]);
}
function testBankManagerPresentsGroupingSelectionAsActive() {
  var viewModel = {
    enabled: true,
    rows: [{
      instanceId: "instance.1",
      label: "Local",
      local: true,
      banks: [{
        bankId: 1,
        active: false,
        selected: true,
      }],
    }],
    groupAction: { enabled: true, active: true },
    clearAction: { enabled: false, armed: false },
    subscribe: function () { return function () {}; },
  };
  var presenter = new BankManagerPresenter(viewModel);

  assert.strictEqual(presenter.presentation.rows[0].banks[0].selected, true);
  assert.strictEqual(presenter.presentation.rows[0].banks[0].active, false);
  presenter.destroy();
}
function testDialUsesPhysicalRangeOutsideGroupScope() {
  var source = {
    value: 0.5,
    physicalMinimum: 0,
    physicalMaximum: 1,
    minimum: 0.5,
    maximum: 1,
  };
  var scope = {
    mode: "local",
    enabled: true,
    isGroup: function () { return this.mode === "group"; },
  };
  var presenter = new DialPresenter({
    rings: [{ value: source }],
    scope: scope,
  });

  assert.strictEqual(presenter.presentation.rings[0].minimum, 0);
  assert.strictEqual(presenter.presentation.rings[0].maximum, 1);

  scope.mode = "group";
  scope.listeners = [];
  presenter.rebuild();
  assert.strictEqual(presenter.presentation.rings[0].minimum, 0.5);
  assert.strictEqual(presenter.presentation.rings[0].maximum, 1);
  presenter.destroy();
}
function testManagedMarkerReachesBankManagerControlBinding() {
  var registry = {
    subscribe: function () { return function () {}; },
    fetch: function () {},
    get: function () { return null; },
  };
  var snapshot = {
    revision: 1,
    instances: [{
      instanceId: "instance.1",
      label: "Local",
      mute: false,
      solo: false,
      processors: [{
        processorId: "equalizer",
        effectActive: true,
        markerActive: true,
        bypassed: false,
        soloed: false,
      }],
      banks: [{ bankId: 0, groupId: null, effectActive: true }],
    }],
    groups: [],
  };
  var viewModel = new BankManagerViewModel(registry, "instance.1");
  viewModel.applyRegistrySnapshot(snapshot);
  var presenter = new BankManagerPresenter(viewModel);
  var messages = [];
  var binding = new BankManagerControlBinding(
    { handleIntent: function () {} },
    presenter,
    function (selector, args) { messages.push([selector, args]); },
  );

  var initialProcessor = messages.filter(function (message) {
    return message[0] === "processor";
  })[0];
  assert.deepStrictEqual(
    initialProcessor[1].slice(0, 4),
    [0, "equalizer", 1, 1],
  );

  messages = [];
  snapshot.instances[0].processors[0].markerActive = false;
  viewModel.applyRegistryUpdate(snapshot, {
    selector: "registry_processor_markers_changed",
    args: [1, 1, "instance.1", 1, "equalizer", 0],
    instanceIds: ["instance.1"],
  });
  var processorPatch = messages.filter(function (message) {
    return message[0] === "processor_patch";
  })[0];
  assert.deepStrictEqual(
    processorPatch[1].slice(0, 4),
    [0, "equalizer", 1, 0],
  );

  binding.destroy();
  presenter.destroy();
  viewModel.destroy();
}
testDoubleClickTrackerRecognizesOnlyTheSameControl();
testControlBindingsDispatchByControlId();
testDialBindingUsesMessageTransportAndIntents();
testDialBindingResumesOnlyTheLatestPresentation();
testDialBindingWrapsGesturesInTransactions();
testDialBindingReportsRejectedTransactionWithoutWriting();
testButtonBindingPreservesPresentationMetadata();
testBankManagerBindingPatchesRegistryAddition();
testUiHostRoutesIntentsByControlVarname();
testPanelBindingHostRoutesListMessagesToNamedControl();
testUiHostEntrypointInitializesFromLiveReadyAndRoutesLists();
testAnalyzerHandlesPublishOnlyAfterTargetSnapshotIsReady();
testAnalyzerPublishesOneSpectrumNotificationPerFftFrame();
testAnalyzerDragClampsToEffectivePeerRanges();
testAnalyzerParameterUpdatesRecalculateLocalCurves();
testAnalyzerGestureKeepsPreviewUntilFinalWriteCompletes();
testAnalyzerConfigurationRecalculatesCurvesForSampleRate();
testEqualizerPresenterBuildsAllBanksCurveFromRawState();
testDetectorPresenterBuildsFilterCurves();
testDetectorPresenterUpdatesCurvesDuringFilterDrag();
testAnalyzerRendersFixedQFilterWithoutEditableQ();
testAnalyzerRendersLegacyHighShelfAndTiltResponses();
testDetectorBindingPublishesCurvesToControl();
testAnalyzerControlPreservesCurvesAcrossHandlePresentation();
testAnalyzerBindingUsesOneTransactionForHandleDrag();
testAnalyzerStartsNextHistoryPointBeforePreviousWriteResponse();
testAnalyzerTargetTransitionCancelsGestureAndTransaction();
testAnalyzerHandleDragPublishesLatestPositionWhileDragging();
testUiHostAcceptsTrackNameMessage();
testUiHostPublishesOnlyChangedInstanceActivity();
testUiHostRefreshesSnapshotBeforePresentingActivatedInstance();
testUiApplicationLoadsAsCommonJsV8Module();
testFeaturePresenterSetEnumeratesTypedPresenters();
testFeaturePresenterSetTracksSourceAvailability();
testPresentationObservableBatchesOneRebuildAndStopsAfterDestroy();
testDialDisplayScaleDoesNotChangePhysicalValue();
testDialUsesPhysicalRangeOutsideGroupScope();
testConsolidatorInitializesDetectorState();
testFilterPositionUsesOneStateBatch();
testEqualizerPositionForwardsFinalWriteCallback();
testDetectorPositionUsesOneStateBatch();
testDetectorBypassIsInvertedForPresentation();
testMessageControlsConstructCompletePresentation();
testBankManagerForwardsShiftSelection();
testBankManagerPanelClickWaitsForSnapshot();
testPanelTransitionAppliesSelectionAfterSnapshot();
testBankManagerForwardsInstanceControlModifiers();
testBankManagerEqualizerResetReachesStateClient();
testBankManagerPresentsGroupingSelectionAsActive();
testManagedMarkerReachesBankManagerControlBinding();
console.log("UiBindingTests passed");
