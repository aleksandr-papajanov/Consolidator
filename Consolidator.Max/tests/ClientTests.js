var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");
global.include = function () {};

var root = path.resolve(__dirname, "..");
[
    "js/Clients/NativeProtocolClient.js",
    "js/Clients/StateClient.js",
    "js/Clients/AnalysisClient.js",
    "js/Clients/RegistryClient.js",
    "js/Clients/ConsolidatorClient.js",
    "js/ViewModels/ObservableValue.js",
    "js/ViewModels/StateValueViewModel.js",
    "js/ViewModels/FilterViewModel.js",
    "js/ViewModels/DetectorFilterViewModel.js",
    "js/ViewModels/BankViewModel.js",
    "js/ViewModels/BankManagerViewModel.js",
    "js/ViewModels/GainViewModel.js",
    "js/ViewModels/CompressorViewModel.js",
    "js/ViewModels/SaturatorViewModel.js",
    "js/ViewModels/EqualizerViewModel.js",
    "js/ViewModels/AnalyzerViewModel.js",
    "js/ViewModels/ConsolidatorViewModel.js",
    "js/Presenters/Core/PresentationObservable.js",
    "js/Presenters/Core/PresentationBinding.js",
    "js/Presenters/Core/Normalization.js",
    "js/Presenters/Dial/DialPresentation.js",
    "js/Presenters/Dial/DialPresenter.js",
    "js/Presenters/Button/ButtonPresentation.js",
    "js/Presenters/Button/ButtonPresenter.js",
    "js/Presenters/Analyzer/AnalyzerPresentation.js",
    "js/Presenters/Analyzer/AnalyzerPresenter.js",
    "js/Controllers/AnalyzerController.js",
    "js/Controllers/FeaturePresenterSet.js",
    "js/Controllers/EqualizerController.js",
    "js/Controllers/BankManagerContext.js",
    "js/Controllers/BankManagerController.js",
    "js/Bindings/ControlBinding.js",
    "js/Bindings/DialControlBinding.js",
    "js/Bindings/ButtonControlBinding.js",
    "js/Bindings/AnalyzerControlBinding.js",
    "js/Bindings/BankManagerControlBinding.js",
    "js/Bindings/ControlBindings.js",
    "js/ConsolidatorUiHost.js"
].forEach(function (file) {
    vm.runInThisContext(
        fs.readFileSync(path.join(root, file), "utf8"),
        { filename: file }
    );
});

function testStateRoundTrip() {
    var sent = [];
    var client = new ConsolidatorClient("ui.main", function (frame) {
        sent.push(frame);
    });
    var response = null;
    client.state.fetch("compressor.threshold", function (entry, error) {
        response = { entry: entry, error: error };
    });

    assert.deepStrictEqual(sent[0], [
        "read", 1, "ui.main", "1", 1, "query", "compressor", "threshold"
    ]);
    client.handleControl("state_begin", [1, "ui.main", "1", "3", 0, 1]);
    client.handleControl("state_entry", [
        1, "ui.main", "1", "3", 0,
        "compressor", "threshold",
        -18, "applied", -60, 0, -30, -5
    ]);
    client.handleControl("state_done", [1, "ui.main", "1", "3"]);

    assert.strictEqual(response.error, null);
    assert.strictEqual(response.entry.value, -18);
    assert.strictEqual(
        client.state.getFor("3", "compressor.threshold").value,
        -18
    );
}

function testAnalysisViewFiltering() {
    var sent = [];
    var client = new ConsolidatorClient("ui.main", function (frame) {
        sent.push(frame);
    });
    var values = [];
    client.analysis.subscribe("spectrum.main", function (value) {
        values.push(value);
    });

    client.analysis.view("7", 2);
    assert.deepStrictEqual(sent[0], ["analysis_view", "7", 2]);
    client.handleAnalysis("spectrum_main", ["10", "8", 2, 1, 2]);
    assert.strictEqual(values.length, 0);
    client.handleAnalysis("spectrum_main", ["10", "7", 2, 1, 2]);
    assert.deepStrictEqual(values[0].values, [1, 2]);
    client.handleAnalysis("spectrum_main", ["9", "7", 2, 3, 4]);
    assert.strictEqual(values.length, 1);
    client.handleAnalysis("spectrum_main", ["11", "7", 2, 5, 6]);
    assert.strictEqual(values[1], null);
    assert.deepStrictEqual(values[2].values, [5, 6]);
}

function testDetectorAnalysisFrames() {
    var client = new ConsolidatorClient("ui.main", function () {});
    var filters = [];
    var combined = [];
    client.analysis.view("7", 2);
    client.analysis.subscribe("detector.compressor.filter.1", function (value) {
        filters.push(value);
    });
    client.analysis.subscribe("detector.saturator.combined", function (value) {
        combined.push(value);
    });
    client.handleAnalysis("detector_filter", [
        "10", "7", 2, "compressor", 1, 1, 1, 2
    ]);
    client.handleAnalysis("detector_combined", [
        "10", "7", 2, "saturator", 1, 3, 4
    ]);
    assert.deepStrictEqual(filters[0].values, [1, 2]);
    assert.deepStrictEqual(combined[0].values, [3, 4]);
    assert.strictEqual(filters[0].active, true);
    assert.strictEqual(combined[0].active, true);
}

function testControlBindingsDispatchByControlId() {
    var calls = [];
    var bindings = new ControlBindings();
    bindings.add("compressor_threshold", {
        handleIntent: function (intent, values) {
            calls.push([intent, values]);
        },
        destroy: function () {}
    });

    bindings.handle("compressor_threshold", "valueChanged", [0, 0.5]);
    bindings.handle("compressor_ratio", "valueChanged", [0, 4]);

    assert.deepStrictEqual(calls, [["valueChanged", [0, 0.5]]]);
    assert.throws(function () {
        bindings.add("compressor_threshold", {
            handleIntent: function () {},
            destroy: function () {}
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
        rings: [{ minimum: 0, maximum: 1, value: 0.5 }]
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
        }
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
        ["set", [0, 0.5]]
    ]);
    binding.handleIntent("valueChanged", [0, 0.75]);
    binding.handleIntent("reset", [0]);
    binding.handleIntent("gestureBegan", [0]);
    binding.handleIntent("gestureEnded", [0]);
    assert.deepStrictEqual(intents, [
        ["setValue", 0, 0.75],
        ["resetValue", 0],
        ["beginGesture", 0],
        ["endGesture", 0]
    ]);
    binding.destroy();
}

function testButtonBindingPreservesPresentationMetadata() {
    var messages = [];
    var presenter = {
        subscribe: function (callback, immediate) {
            if (immediate) callback({
                value: true,
                enabled: true,
                active: true,
                mode: "momentary",
                label: "SOLO"
            });
            return function () {};
        }
    };
    var binding = new ButtonControlBinding(presenter, function (name, args) {
        messages.push([name, args]);
    });

    assert.deepStrictEqual(messages, [
        ["set", [1]],
        ["enabled", [1]],
        ["active", [1]],
        ["mode", ["momentary"]],
        ["label", ["SOLO"]]
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
            destroy: function () {}
        };
    });

    host.handleUiIntent("input_gain", "valueChanged", [0, 0.75]);
    assert.deepStrictEqual(calls, [["valueChanged", [0, 0.75]]]);
    host.bindings.destroy();
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
        outlet: function () {}
    });
    var includeStack = [];
    context.include = function (relativePath) {
        var base = includeStack.length
            ? path.dirname(includeStack[includeStack.length - 1])
            : path.join(root, "js");
        run(path.resolve(base, relativePath));
    };
    function run(file) {
        includeStack.push(file);
        vm.runInContext(fs.readFileSync(file, "utf8"), context, {
            filename: path.relative(root, file)
        });
        includeStack.pop();
    }

    run(path.join(root, "js/ConsolidatorUiHost.js"));
    var host = new context.ConsolidatorUiHost("test.ui", function () {},
        function () {});
    assert.strictEqual(host.lifecycle, "created");
    host.mapping = context.ConsolidatorControlMapping;
    host.bindControls();
    assert.deepStrictEqual(
        Object.keys(host.bindings.items).sort(),
        Object.keys(context.ConsolidatorControlMapping).map(function (key) {
            return context.ConsolidatorControlMapping[key];
        }).sort()
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
        ["bypass", "object", "button"]
    ]);
    set.destroy();
}

function testDialDisplayScaleDoesNotChangePhysicalValue() {
    var value = {
        value: 0.5,
        physicalMinimum: 0,
        physicalMaximum: 1,
        set: function (next) { this.value = next; }
    };
    var presenter = new DialPresenter({
        rings: [{
            value: value,
            display: { decimals: 1, suffix: "%", scale: 100 }
        }]
    });

    assert.strictEqual(presenter.presentation.rings[0].display.value, "50.0%");
    presenter.setValue(0, 0.25);
    assert.strictEqual(value.value, 0.25);
    presenter.destroy();
}

function testAnalysisRejectsLateFrameAfterReturningToView() {
    var client = new ConsolidatorClient("ui.main", function () {});
    var values = [];
    client.analysis.subscribe("spectrum.main", function (value) {
        values.push(value);
    });

    client.analysis.view("a", 2);
    client.handleAnalysis("spectrum_main", [10, "a", 2, 1]);
    client.analysis.view("b", 2);
    client.handleAnalysis("spectrum_main", [11, "b", 2, 2]);
    client.analysis.view("a", 2);
    client.handleAnalysis("spectrum_main", [10, "a", 2, 3]);
    client.handleAnalysis("spectrum_main", [12, "a", 2, 4]);

    assert.strictEqual(values.length, 5);
    assert.strictEqual(values[3], null);
    assert.deepStrictEqual(values[4].values, [4]);
}

function makeStateFixture() {
    var paths = [];
    var batches = [];
    return {
        paths: paths,
        batches: batches,
        subscribe: function () { return function () {}; },
        set: function () {},
        setMany: function (entries) { batches.push(entries); },
        reset: function () {},
        fetch: function (path, callback) {
            callback({ path: path, value: 1 }, null);
        },
        fetchMany: function (requested, callback) {
            requested.forEach(function (path) { paths.push(path); });
            callback({ error: null });
        }
    };
}

function testFilterPositionUsesOneStateBatch() {
    var state = makeStateFixture();
    var filter = new FilterViewModel(state, 2, 4);
    filter.setPosition(750, 3);
    assert.strictEqual(state.batches.length, 1);
    assert.deepStrictEqual(state.batches[0], [
        { path: "equalizer.bank.2.filter.4.frequency", value: 750 },
        { path: "equalizer.bank.2.filter.4.gain", value: 3 }
    ]);
    filter.destroy();
}

function testDetectorPositionUsesOneStateBatch() {
    var state = makeStateFixture();
    var filter = new DetectorFilterViewModel(state, "compressor", 2);
    filter.setPosition(1200, -3);
    assert.deepStrictEqual(state.batches[0], [
        { path: "compressor.detector.filter.2.frequency", value: 1200 },
        { path: "compressor.detector.filter.2.gain", value: -3 }
    ]);
    filter.destroy();
}

function testConsolidatorInitializesDetectorState() {
    var state = makeStateFixture();
    var analysis = { subscribe: function () { return function () {}; } };
    var root = new ConsolidatorViewModel({ state: state, analysis: analysis });
    var error = "not complete";
    root.initialize(function (result) { error = result; });
    assert.strictEqual(error, null);
    assert.ok(state.paths.indexOf("compressor.detector.filter.1.frequency") >= 0);
    assert.ok(state.paths.indexOf("saturator.detector.filter.2.q") >= 0);
    root.destroy();
}

function testAnalyzerPresenterIsReactiveAndSelectable() {
    var spectrum = new ObservableValue();
    var combined = new ObservableValue();
    var frequency = new ObservableValue(1000);
    var gain = new ObservableValue(0);
    var q = new ObservableValue(1);
    var enabled = new ObservableValue(true);
    var presenter = new AnalyzerPresenter({
        spectrum: spectrum,
        combined: combined,
        curves: [],
        parameters: [{ frequency: frequency, gain: gain, q: q, enabled: enabled }]
    });
    var updates = 0;
    presenter.subscribe(function () { updates += 1; });
    spectrum.set({ values: [1] });
    combined.set({ values: [2] });
    presenter.selectFilter(1);
    assert.ok(updates >= 3);
    assert.strictEqual(presenter.presentation.handles[0].selected, true);
    presenter.destroy();
}

function testEqualizerControllerRebindsOnBankChange() {
    var state = makeStateFixture();
    var root = new ConsolidatorViewModel({
        state: state,
        analysis: { subscribe: function () { return function () {}; } }
    });
    var controller = new EqualizerController(root);
    assert.ok(controller.analyzer.presenter.options.parameters[0].frequency.path.indexOf("bank.1") >= 0);
    root.equalizer.showBank(2);
    assert.ok(controller.analyzer.presenter.options.parameters[0].frequency.path.indexOf("bank.2") >= 0);
    root.analyzer.filterCurves[0].set({ active: true, values: [0] });
    assert.deepStrictEqual(
        controller.analyzer.presenter.presentation.curves[0].values,
        [0.5]
    );
    controller.destroy();
    root.destroy();
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

function testAnalyzerControlEndsDragOnRelease() {
    global.mgraphics = { init: function () {}, redraw: function () {} };
    global.outlet = function () {};
    [
        "js/Controls/Analyzer/AnalyzerViewState.js",
        "js/Controls/Analyzer/AnalyzerLayout.js",
        "js/Controls/Analyzer/AnalyzerRenderer.js"
    ].forEach(function (file) {
        vm.runInThisContext(
            fs.readFileSync(path.join(root, file), "utf8"),
            { filename: file }
        );
    });
    vm.runInThisContext(
        fs.readFileSync(path.join(root, "js/Controls/Analyzer/AnalyzerControl.js"), "utf8"),
        { filename: "js/Controls/Analyzer/AnalyzerControl.js" }
    );
    var intents = [];
    analyzerControl.state.dragging = true;
    analyzerControl.state.selectedId = 1;
    analyzerControl.emitIntent = function (name, values) {
        intents.push([name, values]);
    };
    ondrag(0, 0, 0);
    assert.strictEqual(intents[0][0], "gestureEnded");
}

function testMessageControlsConstructCompletePresentation() {
    vm.runInThisContext(
        fs.readFileSync(path.join(root, "js/Controls/Dial/DialControl.js"), "utf8"),
        { filename: "js/Controls/Dial/DialControl.js" }
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
        color: null
    });

    vm.runInThisContext(
        fs.readFileSync(path.join(root, "js/Controls/Button/ButtonControl.js"), "utf8"),
        { filename: "js/Controls/Button/ButtonControl.js" }
    );
    active(1);
    mode("momentary");
    label("SOLO");
    assert.strictEqual(buttonControl.presentation.active, true);
    assert.strictEqual(buttonControl.presentation.mode, "momentary");
    assert.strictEqual(buttonControl.presentation.label, "SOLO");
}

function testRegistrySnapshotRoundTrip() {
    var sent = [];
    var client = new ConsolidatorClient("ui.main", function (frame) {
        sent.push(frame);
    });
    var snapshots = [];
    client.registry.subscribe(function (snapshot) {
        snapshots.push(snapshot);
    });

    client.registry.fetch();
    assert.deepStrictEqual(sent[0], ["registry", 1, "ui.main", "1"]);
    client.handleControl("registry_begin", [1, "ui.main", "1", "20", 1, 1]);
    client.handleControl("registry_instance", [
        1, "ui.main", "1", "7", "Kick", 2
    ]);
    client.handleControl("registry_bank", [
        1, "ui.main", "1", "7", 1, "none"
    ]);
    client.handleControl("registry_bank", [
        1, "ui.main", "1", "7", 2, 0
    ]);
    client.handleControl("registry_group", [1, "ui.main", "1", 0]);
    client.handleControl("registry_member", [
        1, "ui.main", "1", 0, "7", 2
    ]);
    client.handleControl("registry_done", [1, "ui.main", "1"]);

    assert.strictEqual(client.registry.get().revision, 20);
    assert.strictEqual(client.registry.get().instances[0].banks[1].groupId, 0);
    assert.strictEqual(snapshots.length, 1);
}

function testRegistryChangedDuringFetchIsRetained() {
    var sent = [];
    var client = new ConsolidatorClient("ui.main", function (frame) {
        sent.push(frame);
    });

    client.registry.fetch();
    client.handleControl("registry_changed", [1, "21"]);
    client.handleControl("registry_begin", [1, "ui.main", "1", "20", 0, 0]);
    client.handleControl("registry_done", [1, "ui.main", "1"]);
    assert.strictEqual(sent.length, 2);
    assert.deepStrictEqual(sent[1], ["registry", 1, "ui.main", "2"]);

    client.handleControl("registry_begin", [1, "ui.main", "2", "21", 0, 0]);
    client.handleControl("registry_done", [1, "ui.main", "2"]);
    assert.strictEqual(client.registry.get().revision, 21);
}

function testRegistryChangedFetchesWhenIdle() {
    var sent = [];
    var client = new ConsolidatorClient("ui.main", function (frame) {
        sent.push(frame);
    });

    client.handleControl("registry_changed", [1, "5"]);
    assert.deepStrictEqual(sent[0], ["registry", 1, "ui.main", "1"]);
}

function testRegistrySameRevisionDoesNotNotifyAgain() {
    var client = new ConsolidatorClient("ui.main", function () {});
    var notifications = 0;
    client.registry.subscribe(function () {
        notifications += 1;
    });

    client.registry.fetch();
    client.handleControl("registry_begin", [1, "ui.main", "1", "20", 0, 0]);
    client.handleControl("registry_done", [1, "ui.main", "1"]);
    client.registry.fetch();
    client.handleControl("registry_begin", [1, "ui.main", "2", "20", 0, 0]);
    client.handleControl("registry_done", [1, "ui.main", "2"]);

    assert.strictEqual(notifications, 1);
    assert.strictEqual(client.registry.get().revision, 20);
}

function testRegistryIgnoresOtherSource() {
    var client = new ConsolidatorClient("ui.main", function () {});

    client.handleControl("registry_begin", [
        1, "other.ui", "1", "20", 0, 0
    ]);
    client.handleControl("registry_done", [1, "other.ui", "1"]);

    assert.strictEqual(client.registry.get(), null);
}

function testRegistryBroadcastRequiresProtocolVersion() {
    var sent = [];
    var client = new ConsolidatorClient("ui.main", function (frame) {
        sent.push(frame);
    });
    client.handleControl("registry_changed", [2, "99"]);
    assert.strictEqual(sent.length, 0);
    assert.strictEqual(client.registry.requiredRevision, 0);
}

function testRegistryErrorClearsFetchState() {
    var sent = [];
    var client = new ConsolidatorClient("ui.main", function (frame) {
        sent.push(frame);
    });
    var response = null;

    client.registry.fetch(function (snapshot, result) {
        response = { snapshot: snapshot, result: result };
    });
    client.handleControl("error", [
        1, "ui.main", "1", "7", "malformed", "invalid registry request"
    ]);

    assert.strictEqual(client.registry.fetchPending, false);
    assert.strictEqual(response.snapshot, undefined);
    assert.strictEqual(response.result.error, "malformed");

    client.handleControl("registry_changed", [1, "2"]);
    assert.deepStrictEqual(sent[1], ["registry", 1, "ui.main", "2"]);
}

function testBankManagerUsesRegistryAndLocalInstance() {
    var protocol = new NativeProtocolClient("ui.main", function () {});
    var registry = new RegistryClient(protocol);
    var vm = new BankManagerViewModel(registry, "7");
    registry.snapshot = {
        revision: 1,
        instances: [{
            instanceId: 7,
            label: "Kick",
            selectedBank: 2,
            banks: [{ bankId: 1, groupId: null }, { bankId: 2, groupId: 0 }]
        }],
        groups: [{ groupId: 0, members: [{ instanceId: 7, bankId: 2 }] }]
    };
    vm.applyRegistrySnapshot(registry.snapshot);
    assert.strictEqual(vm.rows[0].local, true);
    assert.strictEqual(vm.rows[0].banks[0].bankId, 1);
    assert.strictEqual(vm.rows[0].banks[0].label, "1");
    assert.strictEqual(vm.rows[0].banks[0].system, true);
    assert.strictEqual(vm.rows[0].banks[1].system, false);
    assert.strictEqual(vm.rows[0].banks[1].groupId, 0);
    assert.strictEqual(vm.rows[0].banks[1].system, false);
    assert.strictEqual(vm.linkGroups[0].linkId, 0);
    assert.strictEqual(vm.linkGroups[0].used, true);
    assert.strictEqual(vm.editAction.enabled, true);
    assert.strictEqual(vm.clearAction.enabled, true);
    vm.toggleLinkEditing();
    assert.strictEqual(vm.editAction.active, true);
    vm.destroy();
}

function makeBankManagerControllerFixture(linkEditing) {
    var calls = {
        views: [],
        selected: [],
        toggled: [],
        cleared: 0
    };
    var viewModel = {
        linkEditing: Boolean(linkEditing),
        clearAction: { enabled: true, armed: false },
        apply: function (state) {
            if (state.clearAction) this.clearAction = state.clearAction;
        }
    };
    viewModel.toggleBankSelection = function (instanceId, bankId) {
        calls.toggled.push([instanceId, bankId]);
    };
    viewModel.toggleLinkEditing = function () {};
    viewModel.getSelectedBanks = function () { return []; };
    viewModel.clearBankSelection = function () {};
    var context = new BankManagerContext(
        viewModel,
        {
            setMany: function () { calls.cleared += 1; }
        },
        {
            show: function (instanceId, bankId) {
                calls.views.push([instanceId, bankId]);
            }
        },
        {
            set: function (bankId) { calls.selected.push(bankId); }
        },
        "local"
    );
    return {
        controller: new BankManagerController(context),
        calls: calls,
        viewModel: viewModel
    };
}

function testBankManagerControllerLocalAndRemoteSelection() {
    var fixture = makeBankManagerControllerFixture(false);
    fixture.controller.selectBank("local", 3);
    fixture.controller.selectBank("remote", 4);

    assert.deepStrictEqual(fixture.calls.views, [["local", 3], ["remote", 4]]);
    assert.deepStrictEqual(fixture.calls.selected, [3]);
    assert.deepStrictEqual(fixture.calls.toggled, []);
}

function testBankManagerControllerLinkEditingOnlyTogglesSelection() {
    var fixture = makeBankManagerControllerFixture(true);
    fixture.controller.selectBank("remote", 4);

    assert.deepStrictEqual(fixture.calls.views, []);
    assert.deepStrictEqual(fixture.calls.selected, []);
    assert.deepStrictEqual(fixture.calls.toggled, [["remote", 4]]);
}

function testBankManagerControllerClearRequiresConfirmation() {
    var fixture = makeBankManagerControllerFixture(false);
    var realSetTimeout = setTimeout;
    setTimeout = function () { return null; };
    fixture.controller.clearAll();
    setTimeout = realSetTimeout;
    assert.strictEqual(fixture.viewModel.clearAction.armed, true);
    assert.strictEqual(fixture.calls.cleared, 0);

    fixture.controller.clearAll();
    assert.strictEqual(fixture.viewModel.clearAction.armed, false);
    assert.strictEqual(fixture.calls.cleared, 1);
}

function testBankManagerWritesOnlyLocalSelectedGroups() {
    var writes = [];
    var viewModel = {
        linkEditing: true,
        clearAction: { enabled: true, armed: false },
        getSelectedBanks: function () {
            return [
                { instanceId: "local", bankId: 2 },
                { instanceId: "remote", bankId: 3 }
            ];
        },
        clearBankSelection: function () {},
        toggleLinkEditing: function () {}
    };
    var controller = new BankManagerController(new BankManagerContext(
        viewModel,
        { setMany: function (entries) { writes.push(entries); } },
        { show: function () {} },
        { set: function () {} },
        "local"
    ));

    controller.applyLinkGroup(7);
    assert.deepStrictEqual(writes, [[{
        path: "bank.2.group",
        value: 7
    }]]);
    controller.destroy();
}

testStateRoundTrip();
testAnalysisViewFiltering();
testAnalysisRejectsLateFrameAfterReturningToView();
testDetectorAnalysisFrames();
testControlBindingsDispatchByControlId();
testDialBindingUsesMessageTransportAndIntents();
testButtonBindingPreservesPresentationMetadata();
testUiHostRoutesIntentsByControlVarname();
testUiHostLoadsInIsolatedMaxContext();
testFeaturePresenterSetEnumeratesTypedPresenters();
testDialDisplayScaleDoesNotChangePhysicalValue();
testConsolidatorInitializesDetectorState();
testFilterPositionUsesOneStateBatch();
testDetectorPositionUsesOneStateBatch();
testAnalyzerPresenterIsReactiveAndSelectable();
testEqualizerControllerRebindsOnBankChange();
testDetectorBypassIsInvertedForPresentation();
testAnalyzerControlEndsDragOnRelease();
testMessageControlsConstructCompletePresentation();
testRegistrySnapshotRoundTrip();
testRegistryChangedDuringFetchIsRetained();
testRegistryChangedFetchesWhenIdle();
testRegistrySameRevisionDoesNotNotifyAgain();
testRegistryIgnoresOtherSource();
testRegistryBroadcastRequiresProtocolVersion();
testRegistryErrorClearsFetchState();
testBankManagerUsesRegistryAndLocalInstance();
testBankManagerControllerLocalAndRemoteSelection();
testBankManagerControllerLinkEditingOnlyTogglesSelection();
testBankManagerControllerClearRequiresConfirmation();
testBankManagerWritesOnlyLocalSelectedGroups();
console.log("ClientTests passed");
