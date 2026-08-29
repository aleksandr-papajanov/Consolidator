var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");
var environment = require("../../support/ProductionEnvironment.js");
var root = environment.root;
environment.loadClientEnvironment();
function makeBankManagerControllerFixture() {
  var calls = {
    views: [], selected: [], toggled: [], focused: [], cleared: 0, requests: [],
  };
  var viewModel = {
    clearAction: { enabled: true, armed: false },
    rows: [{ instanceId: "remote", banks: [{ bankId: 5 }] }],
    focusedBank: function () { return { bankId: 5 }; },
    apply: function (state) {
      if (state.clearAction) this.clearAction = state.clearAction;
    },
  };
  viewModel.toggleBankSelection = function (
    instanceId,
    bankId,
    extendSelection
  ) {
    calls.toggled.push([instanceId, bankId, extendSelection]);
  };
  viewModel.canSelectBank = function () { return true; };
  viewModel.setFocusedBank = function (instanceId, bankId) {
    calls.focused.push([instanceId, bankId]);
  };
  viewModel.getSelectedBanks = function () {
    return [];
  };
  viewModel.clearBankSelection = function () {};
  var state = {
    setManyFor: function () {
      calls.cleared += 1;
    },
    setFor: function (instanceId, path, value) {
      calls.selected.push([instanceId, path, value]);
    },
  };
  var uiTarget = {
    show: function (instanceId, bankId) {
      calls.views.push([instanceId, bankId]);
    },
  };
  var protocol = {
    request: function (selector, args) {
      calls.requests.push([selector, args]);
    },
  };
  var context = new BankManagerContext(
    viewModel,
    state,
    uiTarget,
    "local",
    protocol,
  );
  return {
    controller: new BankManagerController(context),
    calls: calls,
    viewModel: viewModel,
  };
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
    1,
    "ui.main",
    "1",
    "7",
    "Kick",
  ]);
  client.handleControl("registry_bank", [1, "ui.main", "1", "7", 1, "none"]);
  client.handleControl("registry_bank", [1, "ui.main", "1", "7", 2, 0]);
  client.handleControl("registry_group", [1, "ui.main", "1", 0]);
  client.handleControl("registry_member", [1, "ui.main", "1", 0, "7", 2]);
  client.handleControl("registry_done", [1, "ui.main", "1"]);

  assert.strictEqual(client.registry.get().revision, 20);
  assert.strictEqual(client.registry.get().instances[0].banks[1].groupId, 0);
  assert.strictEqual(snapshots.length, 1);
}
function testRegistryDeltaDuringFetchIsRetained() {
  var sent = [];
  var client = new ConsolidatorClient("ui.main", function (frame) {
    sent.push(frame);
  });

  client.registry.fetch();
  client.handleControl("registry_instance_added", [
    1, 20, 21, "7", "Kick", 0, 0, 0,
  ]);
  client.handleControl("registry_begin", [1, "ui.main", "1", "20", 0, 0]);
  client.handleControl("registry_done", [1, "ui.main", "1"]);
  assert.strictEqual(sent.length, 2);
  assert.deepStrictEqual(sent[1], ["registry", 1, "ui.main", "2"]);

  client.handleControl("registry_begin", [1, "ui.main", "2", "21", 0, 0]);
  client.handleControl("registry_done", [1, "ui.main", "2"]);
  assert.strictEqual(client.registry.get().revision, 21);
}
function testRegistryDoesNotPublishStaleSnapshotOverLabelDelta() {
  var sent = [];
  var client = new ConsolidatorClient("ui.main", function (frame) {
    sent.push(frame);
  });

  client.registry.snapshot = {
    revision: 10,
    instances: [{ instanceId: "7", label: "Old", banks: [] }],
    groups: [],
  };
  client.registry.requiredRevision = 12;
  client.registry.fetch();
  client.handleControl("registry_begin", [1, "ui.main", "1", "11", 0, 0]);
  client.handleControl("registry_done", [1, "ui.main", "1"]);

  assert.strictEqual(client.registry.get().instances[0].label, "Old");
  assert.strictEqual(sent.length, 2);

  client.handleControl("registry_begin", [1, "ui.main", "2", "12", 1, 1]);
  client.handleControl("registry_instance", [1, "ui.main", "2", "7", "Renamed"]);
  client.handleControl("registry_done", [1, "ui.main", "2"]);

  assert.strictEqual(client.registry.get().instances[0].label, "Renamed");
}
function testRegistryDeltaFetchesWhenIdle() {
  var sent = [];
  var client = new ConsolidatorClient("ui.main", function (frame) {
    sent.push(frame);
  });

  client.handleControl("registry_instance_added", [
    1, 4, 5, "7", "Kick", 0, 0, 0,
  ]);
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

function testRegistryDeltaUpdatesOnlyTheAffectedRow() {
  var protocol = new NativeProtocolClient("ui.main", function () {});
  var registry = new RegistryClient(protocol);
  registry.snapshot = {
    revision: 3,
    instances: [
      { instanceId: "1", label: "One", banks: [{ bankId: 1, groupId: null }] },
      { instanceId: "2", label: "Two", banks: [{ bankId: 1, groupId: null }] },
    ],
    groups: [],
  };
  var viewModel = new BankManagerViewModel(registry, "1");
  viewModel.applyRegistrySnapshot(registry.snapshot);
  var firstRow = viewModel.rows[0];
  var secondRow = viewModel.rows[1];

  viewModel.applyRegistryUpdate(registry.snapshot, {
    selector: "registry_label_changed",
    args: [1, 3, 4, "2", "Renamed"],
  });

  assert.strictEqual(viewModel.rows[0], firstRow);
  assert.strictEqual(viewModel.rows[1], secondRow);
  assert.strictEqual(viewModel.rows[1].label, "Renamed");
  assert.strictEqual(viewModel.rows[0].label, "One");
  viewModel.destroy();
}
function testRegistryUsesNativeDeliveryIdentity() {
  var client = new ConsolidatorClient("ui.main", function () {});

  client.registry.fetch();
  client.handleControl("registry_begin", [1, "7", "1", "20", 0, 0]);
  client.handleControl("registry_done", [1, "7", "1"]);

  assert.strictEqual(client.registry.get().revision, 20);
}
function testRegistryBroadcastRequiresProtocolVersion() {
  var sent = [];
  var client = new ConsolidatorClient("ui.main", function (frame) {
    sent.push(frame);
  });
  client.handleControl("registry_instance_added", [
    2, 98, 99, "7", "Kick", 0, 0, 0,
  ]);
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
    1,
    "ui.main",
    "1",
    "malformed",
    "invalid registry request",
  ]);

  assert.strictEqual(client.registry.fetchPending, false);
  assert.strictEqual(response.snapshot, undefined);
  assert.strictEqual(response.result.error, "malformed");

  client.handleControl("registry_instance_added", [
    1, 1, 2, "7", "Kick", 0, 0, 0,
  ]);
  assert.deepStrictEqual(sent[1], ["registry", 1, "ui.main", "2"]);
}
function testStateClientAddressesRemoteTopologyWrites() {
  var sent = [];
  var protocol = new NativeProtocolClient("ui.main", function (frame) {
    sent.push(frame);
  });
  var state = new StateClient(protocol);

  state.setFor("7", "bank.3.group", 5);

  assert.deepStrictEqual(sent[0], [
    "write",
    1,
    "ui.main",
    "1",
    "7",
    "0",
    1,
    "entry",
    "bank",
    3,
    "group",
    "value",
    5,
  ]);
}
function testStateClientEncodesZeroBasedBankPath() {
  var sent = [];
  var protocol = new NativeProtocolClient("ui.main", function (frame) {
    sent.push(frame);
  });
  var state = new StateClient(protocol);

  state.setFor("7", "bank.0.group", 5);

  assert.deepStrictEqual(sent[0].slice(7, 12), [
    "entry", "bank", 0, "group", "value",
  ]);
}
function testBankManagerUsesRegistryAndLocalInstance() {
  var protocol = new NativeProtocolClient("ui.main", function () {});
  var registry = new RegistryClient(protocol);
  var vm = new BankManagerViewModel(registry, "7");
  registry.snapshot = {
    revision: 1,
    instances: [
      {
        instanceId: 7,
        label: "Kick",
        banks: [
          { bankId: 0, groupId: null },
          { bankId: 1, groupId: 1 },
        ],
      },
    ],
    groups: [{ groupId: 1, members: [{ instanceId: 7, bankId: 1 }] }],
  };
  vm.applyRegistrySnapshot(registry.snapshot);
  assert.strictEqual(vm.rows[0].local, true);
  assert.strictEqual(vm.rows[0].banks[0].bankId, 0);
  assert.strictEqual(vm.rows[0].banks[0].label, "0");
  assert.strictEqual(vm.rows[0].banks[0].system, false);
  assert.strictEqual(vm.rows[0].banks[1].system, false);
  assert.strictEqual(vm.rows[0].banks[1].groupId, 1);
  assert.strictEqual(vm.rows[0].banks[1].active, false);
  assert.strictEqual(vm.rows[0].banks[1].system, false);
  assert.strictEqual(vm.ungroupAction.enabled, false);
  assert.strictEqual(vm.clearAction.enabled, true);
  vm.setFocusedBank(7, 1);
  assert.strictEqual(vm.rows[0].banks[1].focused, true);
  assert.strictEqual(vm.ungroupAction.enabled, true);
  vm.destroy();
}
function testBankManagerOffersNextGroupId() {
  var protocol = new NativeProtocolClient("ui.main", function () {});
  var registry = new RegistryClient(protocol);
  var vm = new BankManagerViewModel(registry, "7");
  registry.snapshot = {
    revision: 1,
    instances: [],
    groups: [],
  };

  vm.applyRegistrySnapshot(registry.snapshot);

  assert.strictEqual(vm.nextGroupId(), 1);
  assert.strictEqual(vm.groupAction.enabled, false);
  vm.destroy();
}
function testBankManagerKeepsOneFocusedBankAcrossInstances() {
  var protocol = new NativeProtocolClient("ui.main", function () {});
  var registry = new RegistryClient(protocol);
  var vm = new BankManagerViewModel(registry, "local");
  registry.snapshot = {
    revision: 1,
    instances: [
      {
        instanceId: "local",
        label: "Local",
        banks: [{ bankId: 1 }, { bankId: 2 }],
      },
      {
        instanceId: "remote",
        label: "Remote",
        banks: [{ bankId: 1 }, { bankId: 2 }],
      },
    ],
    groups: [],
  };
  vm.setFocusedBank("local", 2);
  assert.strictEqual(vm.rows[0].banks[1].focused, true);
  assert.strictEqual(vm.rows[1].banks[1].focused, false);
  vm.setFocusedBank("remote", 2);
  assert.strictEqual(vm.rows[0].banks[1].focused, false);
  assert.strictEqual(vm.rows[1].banks[1].focused, true);
  vm.destroy();
}
function testBankManagerControllerLocalAndRemoteSelection() {
  var fixture = makeBankManagerControllerFixture(false);
  fixture.controller.selectBank("local", 3);
  fixture.controller.selectBank("remote", 4);

  assert.deepStrictEqual(fixture.calls.views, [
    ["local", 3],
    ["remote", 4],
  ]);
  assert.deepStrictEqual(fixture.calls.selected, []);
  assert.deepStrictEqual(fixture.calls.focused, [
    ["local", 3],
    ["remote", 4],
  ]);
  assert.deepStrictEqual(fixture.calls.toggled, [
    ["local", 3, false],
    ["remote", 4, false],
  ]);

  fixture.controller.selectRow("remote");
  assert.deepStrictEqual(fixture.calls.views, [
    ["local", 3],
    ["remote", 4],
    ["remote", 5],
  ]);

  assert.deepStrictEqual(fixture.calls.selected, []);
}
function testBankManagerControllerShiftOnlyTogglesSelection() {
  var fixture = makeBankManagerControllerFixture();
  fixture.controller.handleIntent("bankSelected", ["remote", 4, 1]);

  assert.deepStrictEqual(fixture.calls.views, []);
  assert.deepStrictEqual(fixture.calls.selected, []);
  assert.deepStrictEqual(fixture.calls.toggled, [["remote", 4, true]]);
}
function testBankManagerControllerSendsExplicitInstanceAndGroupControls() {
  var fixture = makeBankManagerControllerFixture();

  fixture.controller.handleIntent("instanceMuteChanged", ["remote", 1, 0]);
  fixture.controller.handleIntent("instanceMuteChanged", ["remote", 0, 1]);
  fixture.controller.handleIntent("instanceSoloChanged", ["remote", 1, 0, 0]);
  fixture.controller.handleIntent("instanceSoloChanged", ["remote", 1, 1, 1]);

  assert.deepStrictEqual(fixture.calls.requests, [
    ["set_instance_mute", ["remote", "instance", 1]],
    ["set_instance_mute", ["remote", "group", 5, 0]],
    ["set_instance_solo", ["remote", "instance", 1, "exclusive"]],
    ["set_instance_solo", ["remote", "group", 5, 1, "additive"]],
  ]);
}
function testBankManagerShiftExtendsGroupingSelection() {
  var protocol = new NativeProtocolClient("ui.main", function () {});
  var registry = new RegistryClient(protocol);
  var viewModel = new BankManagerViewModel(registry, "local");
  registry.snapshot = {
    revision: 1,
    instances: [{
      instanceId: "local",
      label: "Local",
      banks: [{ bankId: 1 }, { bankId: 2 }, { bankId: 3 }],
    }],
    groups: [],
  };
  viewModel.applyRegistrySnapshot(registry.snapshot);
  viewModel.toggleBankSelection("local", 1, false);
  viewModel.toggleBankSelection("local", 2, true);
  assert.deepStrictEqual(viewModel.getSelectedBanks(), [
    { instanceId: "local", bankId: 1 },
  ]);

  viewModel.toggleBankSelection("local", 3, false);
  assert.deepStrictEqual(viewModel.getSelectedBanks(), [
    { instanceId: "local", bankId: 3 },
  ]);
  assert.strictEqual(viewModel.rows[0].banks[2].selected, true);
  viewModel.destroy();
}
function testBankManagerControllerClearRequiresConfirmation() {
  var fixture = makeBankManagerControllerFixture(false);
  fixture.viewModel.rows[0].local = true;
  fixture.viewModel.rows[0].banks[0].groupId = 1;
  var realSetTimeout = setTimeout;
  setTimeout = function () {
    return null;
  };
  fixture.controller.clearLocalGroups();
  setTimeout = realSetTimeout;
  assert.strictEqual(fixture.viewModel.clearAction.armed, true);
  assert.strictEqual(fixture.calls.cleared, 0);

  fixture.controller.clearLocalGroups();
  assert.strictEqual(fixture.viewModel.clearAction.armed, false);
  assert.strictEqual(fixture.calls.cleared, 1);
}
function testBankManagerWritesSelectedGroupsForEveryInstance() {
  var writes = [];
  var viewModel = {
    clearAction: { enabled: true, armed: false },
    nextGroupId: function () { return 7; },
    getSelectedBanks: function () {
      return [
        { instanceId: "local", bankId: 2 },
        { instanceId: "remote", bankId: 3 },
      ];
    },
    clearBankSelection: function () {},
  };
  var controller = new BankManagerController(
    new BankManagerContext(
      viewModel,
      { setManyFor: function (instanceId, entries) {
          writes.push([instanceId, entries]);
        },
        setFor: function () {},
      },
      {
        show: function () {},
      },
      "local",
    ),
  );

  controller.groupSelectedBanks();
  assert.deepStrictEqual(writes, [
    [
      "local",
      [{
        path: "bank.2.group",
        value: 7,
      }],
    ],
      [
        "remote",
        [{
          path: "bank.3.group",
          value: 7,
        }],
      ],
  ]);
  controller.destroy();
}
testRegistrySnapshotRoundTrip();
testRegistryDeltaDuringFetchIsRetained();
testRegistryDoesNotPublishStaleSnapshotOverLabelDelta();
testRegistryDeltaFetchesWhenIdle();
testRegistrySameRevisionDoesNotNotifyAgain();
testRegistryDeltaUpdatesOnlyTheAffectedRow();
testRegistryUsesNativeDeliveryIdentity();
testRegistryBroadcastRequiresProtocolVersion();
testRegistryErrorClearsFetchState();
testStateClientAddressesRemoteTopologyWrites();
testStateClientEncodesZeroBasedBankPath();
testBankManagerUsesRegistryAndLocalInstance();
testBankManagerOffersNextGroupId();
testBankManagerKeepsOneFocusedBankAcrossInstances();
testBankManagerControllerLocalAndRemoteSelection();
testBankManagerControllerShiftOnlyTogglesSelection();
testBankManagerControllerSendsExplicitInstanceAndGroupControls();
testBankManagerShiftExtendsGroupingSelection();
testBankManagerControllerClearRequiresConfirmation();
testBankManagerWritesSelectedGroupsForEveryInstance();
console.log("RegistryAndBankManagerTests passed");
