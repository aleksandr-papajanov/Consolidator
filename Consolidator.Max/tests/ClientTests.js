var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");

var root = path.resolve(__dirname, "..");
[
    "js/Clients/NativeProtocolClient.js",
    "js/Clients/StateClient.js",
    "js/Clients/AnalysisClient.js",
    "js/Clients/RegistryClient.js",
    "js/Clients/ConsolidatorClient.js",
    "js/ViewModels/BankManagerViewModel.js",
    "js/Controllers/BankManagerController.js"
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
    var rootViewModel = {
        instanceId: "local",
        analyzer: {
            show: function (instanceId, bankId) {
                calls.views.push([instanceId, bankId]);
            }
        },
        selectedBank: {
            set: function (bankId) { calls.selected.push(bankId); }
        },
        bankSelection: {
            toggle: function (instanceId, bankId) {
                calls.toggled.push([instanceId, bankId]);
            }
        },
        clearAllBanks: function () { calls.cleared += 1; }
    };
    return {
        controller: new BankManagerController(viewModel, rootViewModel),
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

testStateRoundTrip();
testAnalysisViewFiltering();
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
console.log("ClientTests passed");
