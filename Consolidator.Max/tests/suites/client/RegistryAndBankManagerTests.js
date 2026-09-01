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
    setManyTopologyFor: function () {
      calls.cleared += 1;
    },
    set: function (path, value) {
      calls.selected.push([path, value]);
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
  client.handleControl("registry_processor", [
    1, "ui.main", "1", "7", "input", 1, 1, 0, 0,
  ]);
  client.handleControl("registry_processor", [
     1, "ui.main", "1", "7", "equalizer", 1, 0, 0,
  ]);
  client.handleControl("registry_bank", [1, "ui.main", "1", "7", 1, "none"]);
  client.handleControl("registry_bank", [1, "ui.main", "1", "7", 2, 0]);
  client.handleControl("registry_group", [1, "ui.main", "1", 0]);
  client.handleControl("registry_member", [1, "ui.main", "1", 0, "7", 2]);
  client.handleControl("registry_done", [1, "ui.main", "1"]);

  assert.strictEqual(client.registry.get().revision, 20);
  assert.strictEqual(client.registry.get().instances[0].banks[1].groupId, 0);
  assert.strictEqual(client.registry.get().instances[0].processors[0].markerActive, true);
  assert.strictEqual(client.registry.get().instances[0].processors[1].markerActive, false);
  assert.strictEqual(snapshots.length, 1);

  client.handleControl("registry_processor_markers_changed", [
    1, 1, "7", 1, "equalizer", 1,
  ]);
  assert.strictEqual(client.registry.get().instances[0].processors[1].markerActive, true);
  assert.strictEqual(snapshots.length, 2);

  client.handleControl("registry_processor_markers_changed", [
    1, 1, "7", 1, "equalizer",
  ]);
  assert.strictEqual(client.registry.get().instances[0].processors[1].markerActive, true);
  assert.strictEqual(snapshots.length, 2);
}

function testRegistryProcessorDeltaPatchesOneRow() {
  var protocol = new NativeProtocolClient("ui.main", function () {});
  var registry = new RegistryClient(protocol);
  registry.snapshot = {
    revision: 4,
    instances: [{
      instanceId: "1",
      label: "One",
      processors: [{
        processorId: "saturator",
        effectActive: false,
         bypassed: true,
      }],
      banks: [],
    }],
    groups: [],
  };
  var viewModel = new BankManagerViewModel(registry, "1");
  viewModel.applyRegistrySnapshot(registry.snapshot);
  viewModel.applyRegistryUpdate(registry.snapshot, {
    selector: "registry_processor_changed",
     args: [1, 4, 5, "1", "saturator", 1, 0],
  });
  assert.strictEqual(viewModel.rows[0].processors[0].effectActive, true);
  assert.strictEqual(viewModel.rows[0].processors[0].bypassed, false);
  viewModel.destroy();
}
function testEqualizerGainRoundTripUpdatesBankActivityMarker() {
  var sent = [];
  var client = new ConsolidatorClient("ui.main", function (frame) {
    sent.push(frame);
  });
  var vm = new BankManagerViewModel(client.registry, "7");
  client.registry.subscribe(function (snapshot, delta) {
    vm.applyRegistryUpdate(snapshot, delta);
  });
  client.registry.snapshot = {
    revision: 10,
    instances: [{
      instanceId: "7",
      label: "Track",
      processors: [],
      banks: [{ bankId: 0, groupId: null, effectActive: false }],
    }],
    groups: [],
  };
  vm.applyRegistrySnapshot(client.registry.snapshot);

  client.state.set(
    "equalizer.bank.0.filter.1.gain",
    4.5,
  );
  assert.deepStrictEqual(sent[0].slice(0, 9), [
    "write", 1, "ui.main", "1", "group", "0", 1,
    "entry", "equalizer",
  ]);

  client.handleControl("registry_bank_effect_changed", [
    1, 10, 11, "7", 0, 1,
  ]);
  assert.strictEqual(vm.rows[0].banks[0].effectActive, true);

  client.handleControl("registry_bank_effect_changed", [
    1, 11, 12, "7", 0, 0,
  ]);
  assert.strictEqual(vm.rows[0].banks[0].effectActive, false);
  vm.destroy();
}
function testBankEffectDeltaSendsInactiveBankPatchToControl() {
  var sent = [];
  var binding = new BankManagerControlBinding(
    {},
    { subscribe: function () { return function () {}; } },
    function (selector, args) {
      sent.push([selector, args]);
    },
  );
  binding.hasPresentation = true;

  var presentation = {
    enabled: true,
    rows: [{
      instanceId: "7",
      label: "Track",
      local: true,
      mute: false,
      solo: false,
      processors: [],
      banks: [{
        bankId: 0,
        label: "0",
        system: false,
        visible: true,
        enabled: true,
        active: false,
        selected: false,
        opacity: 1,
        groupId: null,
        effectActive: false,
      }],
    }],
    delta: {
      selector: "registry_bank_effect_changed",
      args: [1, 11, 12, "7", 0, 0],
      rowIndex: 0,
    },
  };
  binding.applyDelta(presentation, presentation.delta);

  var bankPatch = sent.filter(function (message) {
    return message[0] === "bank_patch";
  })[0];
  assert.ok(bankPatch);
  assert.strictEqual(bankPatch[1][0], 0);
  assert.strictEqual(bankPatch[1][1], 0);
  assert.strictEqual(bankPatch[1][10], 0);
  assert.deepStrictEqual(sent[sent.length - 1], ["presentation_patch_end", []]);
  binding.destroy();
}

function testViewModelUsesManagedProcessorMarkers() {
  var registry = { subscribe: function () { return function () {}; }, fetch: function () {} };
  var vm = new BankManagerViewModel(registry, "local");
  var snapshot = {
    revision: 1,
    instances: [{
      instanceId: "local",
      label: "Local",
      processors: [
         { processorId: "saturator", effectActive: false, markerActive: false, bypassed: false },
         { processorId: "equalizer", effectActive: false, markerActive: true, bypassed: false },
      ],
      banks: [
        { bankId: 1, groupId: null, effectActive: true },
        { bankId: 2, groupId: 4, effectActive: false },
      ],
    }, {
      instanceId: "remote",
      label: "Remote",
      processors: [
         { processorId: "saturator", effectActive: true, markerActive: true, bypassed: false },
         { processorId: "equalizer", effectActive: true, markerActive: false, bypassed: false },
      ],
      banks: [
        { bankId: 1, groupId: null, effectActive: false },
        { bankId: 2, groupId: 4, effectActive: true },
      ],
    }],
    groups: [],
  };
  vm.applyRegistrySnapshot(snapshot);

  vm.setFocusedBank("local", 1);
  assert.strictEqual(vm.rows[0].processors.filter(function (item) {
    return item.processorId === "equalizer";
  })[0].markerActive, true);
  assert.strictEqual(vm.rows[1].processors.filter(function (item) {
    return item.processorId === "equalizer";
  })[0].markerActive, false);

  vm.setFocusedBank("local", 2);
  assert.strictEqual(vm.rows[0].processors.filter(function (item) {
    return item.processorId === "equalizer";
  })[0].markerActive, true);
  assert.strictEqual(vm.rows[1].processors.filter(function (item) {
    return item.processorId === "equalizer";
  })[0].markerActive, false);

  var topologyDelta = {
    selector: "registry_bank_group_changed",
    args: [1, 1, 2, "remote", 2, "none"],
  };
  vm.applyRegistryUpdate(null, topologyDelta);
  assert.strictEqual(vm.rows[0].processors.filter(function (item) {
    return item.processorId === "equalizer";
  })[0].markerActive, true);

  var markerDelta = {
    selector: "registry_processor_markers_changed",
    args: [1, 1, "local", 1, "equalizer", 0],
    instanceIds: ["local"],
  };
  snapshot.instances[0].processors[1].markerActive = false;
  vm.applyRegistryUpdate(snapshot, markerDelta);
  assert.deepStrictEqual(markerDelta.rowIndices, [0]);
  assert.strictEqual(vm.rows[0].processors.filter(function (item) {
    return item.processorId === "equalizer";
  })[0].markerActive, false);
  vm.destroy();
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
  client.handleControl("registry_begin", [1, "ui.main", "1", "20", 1, 0]);
  client.handleControl("registry_instance", [
    1, "ui.main", "1", "7", "Track", 0, 0,
  ]);
  client.handleControl("registry_processor", [
    1, "ui.main", "1", "7", "equalizer", 1, 0, 0, 0,
  ]);
  client.handleControl("registry_done", [1, "ui.main", "1"]);
  client.registry.fetch();
  client.handleControl("registry_begin", [1, "ui.main", "2", "20", 1, 0]);
  client.handleControl("registry_instance", [
    1, "ui.main", "2", "7", "Track", 0, 0,
  ]);
  client.handleControl("registry_processor", [
    1, "ui.main", "2", "7", "equalizer", 1, 0, 0, 0,
  ]);
  client.handleControl("registry_done", [1, "ui.main", "2"]);

  assert.strictEqual(notifications, 1);
  assert.strictEqual(client.registry.get().revision, 20);

  client.registry.fetch();
  client.handleControl("registry_begin", [1, "ui.main", "3", "20", 1, 0]);
  client.handleControl("registry_instance", [
    1, "ui.main", "3", "7", "Track", 0, 0,
  ]);
  client.handleControl("registry_processor", [
    1, "ui.main", "3", "7", "equalizer", 1, 1, 0, 0,
  ]);
  client.handleControl("registry_done", [1, "ui.main", "3"]);

  assert.strictEqual(notifications, 2);
  assert.strictEqual(
    client.registry.get().instances[0].processors[0].markerActive,
    true,
  );
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

  state.setManyTopologyFor("7", [{ path: "bank.3.group", value: 5 }]);

  assert.deepStrictEqual(sent[0], [
    "write",
    1,
    "ui.main",
    "1",
    "topology",
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

  state.setManyTopologyFor("7", [{ path: "bank.0.group", value: 5 }]);

  assert.deepStrictEqual(sent[0].slice(8, 13), [
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
function testBankManagerControllerSendsRelativeInstanceControlScopes() {
  var fixture = makeBankManagerControllerFixture();

  fixture.controller.handleIntent("instanceMuteChanged", [1, 0]);
  fixture.controller.handleIntent("instanceMuteChanged", [0, 1]);
  fixture.controller.handleIntent("instanceSoloChanged", [1, 0, 0]);
  fixture.controller.handleIntent("instanceSoloChanged", [1, 1, 1]);

  assert.deepStrictEqual(fixture.calls.requests, [
    ["set_instance_mute", ["local", 1]],
    ["set_instance_mute", ["group", 0]],
    ["set_instance_solo", ["local", 1, "exclusive"]],
    ["set_instance_solo", ["group", 1, "additive"]],
  ]);
}

function testBankManagerControllerSendsProcessorControls() {
  var fixture = makeBankManagerControllerFixture();
  fixture.controller.handleIntent("processorBypassChanged", ["compressor", 1, 0]);
  assert.deepStrictEqual(fixture.calls.requests, [
     ["set_processor_bypass", ["compressor", "local", 1]],
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
      { setManyTopologyFor: function (instanceId, entries) {
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
testRegistryProcessorDeltaPatchesOneRow();
testEqualizerGainRoundTripUpdatesBankActivityMarker();
testBankEffectDeltaSendsInactiveBankPatchToControl();
testViewModelUsesManagedProcessorMarkers();
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
testBankManagerControllerSendsRelativeInstanceControlScopes();
testBankManagerControllerSendsProcessorControls();
testBankManagerShiftExtendsGroupingSelection();
testBankManagerControllerClearRequiresConfirmation();
testBankManagerWritesSelectedGroupsForEveryInstance();
console.log("RegistryAndBankManagerTests passed");
