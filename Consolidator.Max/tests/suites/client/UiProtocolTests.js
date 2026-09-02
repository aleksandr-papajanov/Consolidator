var assert = require("assert");
var environment = require("../../support/ProductionEnvironment.js");
environment.loadClientEnvironment();

function testInitializationUsesExternalIdentityFromManaged() {
  var frames = [];
  var client = new ConsolidatorClient("ui", function (frame) {
    frames.push(frame);
  });
  var result;
  client.initialize(function (response) { result = response; });
  assert.deepStrictEqual(frames[0], ["initialize", 1, "ui", "1"]);
  client.handleControl("initialized", [1, "17", "1", "17", "equalizer"]);
  assert.strictEqual(String(result.instanceId), "17");
  assert.strictEqual(result.snapshotContext, "equalizer");
}

function testInstanceActivityUsesTheInstanceCommand() {
  var frames = [];
  var client = new ConsolidatorClient("ui", function (frame) {
    frames.push(frame);
  });

  client.setInstanceActive(true);

  assert.deepStrictEqual(frames[0], [
    "set_instance_active", 1, "ui", "1", 1,
  ]);
}

function testTargetSnapshotAndPushUpdateUseSemanticPaths() {
  var frames = [];
  var client = new ConsolidatorClient("ui", function (frame) {
    frames.push(frame);
  });
  var values = [];
  client.targetState.subscribe("compressor.threshold", function (entry) {
    values.push(entry.value);
  });
  client.uiTarget.show("4", 2, "compressor");
  assert.deepStrictEqual(frames[0], ["observe_target", 1, "ui", "1", "4", 2, "compressor"]);
  client.handleControl("target_state_snapshot", [
    1, "9", "1", "4", 2, "compressor", 1, "compressor.threshold", -24,
    -60, 0, -60, 0,
  ]);
  client.handleControl("state_changed", [
    1, "compressor.threshold", -18, "ready", -60, 0, -60, 0,
  ]);
  assert.deepStrictEqual(values, [-24, -18]);
}

function testUiTargetReportsSnapshotCompletion() {
  var client = new ConsolidatorClient("ui", function () {});
  var response;

  client.uiTarget.show("4", 2, "compressor", function (result) {
    response = result;
  });
  client.handleControl("target_state_snapshot", [
    1, "9", "1", "4", 2, "compressor", 1, "compressor.threshold", -24,
    -60, 0, -60, 0,
  ]);

  assert.strictEqual(response.error, null);
  assert.strictEqual(response.entries.length, 1);
  assert.strictEqual(response.entries[0].value, -24);
  client.destroy();
}

function testObservedEqualizerPathsAreExpandedForWrites() {
  var frames = [];
  var protocol = new NativeProtocolClient("ui", function (frame) {
    frames.push(frame);
  });
  var target = new TargetStateClient(protocol, new StateClient(protocol));
  target.target = { instanceId: "8", bankId: 3 };
    target.set("equalizer.filter.2.gain", 4.5);
    assert.deepStrictEqual(frames[0], [
      "write", 1, "ui", "1", "local", "0", 1,
      "entry", "equalizer", "bank", "filter", 2, "gain", "value", 4.5,
      "copy",
    ]);
}

function testTargetSnapshotNotifiesStateValueOnceAfterCompletion() {
  var client = new ConsolidatorClient("ui", function () {});
  var value = new StateValueViewModel(
    client.targetState,
    "compressor.threshold",
  );
  var notifications = 0;
  value.subscribe(function () { notifications += 1; });
  client.uiTarget.show("4", 2, "compressor");
  notifications = 0;

  client.handleControl("target_state_snapshot", [
    1, "9", "1", "4", 2, "compressor", 1, "compressor.threshold", -24,
    -60, 0, -60, 0,
  ]);
  assert.strictEqual(notifications, 1);
  assert.strictEqual(value.value, -24);
  value.destroy();
  client.destroy();
}

function testStaleTargetSnapshotDoesNotResumeLatestTransition() {
  var client = new ConsolidatorClient("ui", function () {});
  var transitions = [];
  client.targetState.onTargetTransitionBegin(function () {
    transitions.push("begin");
  });
  client.targetState.onTargetTransitionDone(function () {
    transitions.push("done");
  });

  client.uiTarget.show("4", 2, "compressor");
  client.uiTarget.show("4", 3, "compressor");
  client.handleControl("target_state_snapshot", [
    1, "9", "1", "4", 2, "compressor", 1, "compressor.threshold", -24,
    -60, 0, -60, 0,
  ]);
  assert.deepStrictEqual(transitions, ["begin", "begin"]);
  assert.strictEqual(client.targetState.target, null);
  assert.strictEqual(client.targetState.pendingTarget.bankId, 3);

  client.handleControl("target_state_snapshot", [
    1, "9", "2", "4", 3, "compressor", 1, "compressor.threshold", -18,
    -60, 0, -60, 0,
  ]);
  assert.deepStrictEqual(transitions, ["begin", "begin", "done"]);
  assert.strictEqual(client.targetState.target.bankId, 3);
  assert.strictEqual(client.targetState.cache["compressor.threshold"].value, -18);
  client.destroy();
}

function testCallbacklessGestureWritesDoNotAccumulatePendingRequests() {
  var protocol = new NativeProtocolClient("ui", function () {});
  var state = new StateClient(protocol);
  for (var index = 0; index < 100; index += 1) {
    state.setMany([
      { path: "compressor.detector.filter.1.frequency", value: 1000 + index },
      { path: "compressor.detector.filter.1.gain", value: index * 0.01 },
    ], undefined, 42);
  }

  assert.deepStrictEqual(Object.keys(protocol.pending), []);
}

function testWriteWithCallbackIsNotEligibleForGestureCoalescing() {
  var frames = [];
  var protocol = new NativeProtocolClient("ui", function (frame) {
    frames.push(frame);
  });
  var state = new StateClient(protocol);
  var response;
  state.set("compressor.threshold", -18, function (value) {
    response = value;
  }, 42);

  assert.strictEqual(frames[0][4], "local");
  assert.strictEqual(frames[0][5], "42");
  assert.deepStrictEqual(Object.keys(protocol.pending), ["1"]);
  protocol.handleControl("action_done", [1, "ui", "1", 1]);
  assert.strictEqual(response.status, "accepted");
  assert.deepStrictEqual(Object.keys(protocol.pending), []);
}

testInitializationUsesExternalIdentityFromManaged();
testInstanceActivityUsesTheInstanceCommand();
testTargetSnapshotAndPushUpdateUseSemanticPaths();
testUiTargetReportsSnapshotCompletion();
testObservedEqualizerPathsAreExpandedForWrites();
testTargetSnapshotNotifiesStateValueOnceAfterCompletion();
testStaleTargetSnapshotDoesNotResumeLatestTransition();
testCallbacklessGestureWritesDoNotAccumulatePendingRequests();
testWriteWithCallbackIsNotEligibleForGestureCoalescing();
console.log("UiProtocolTests passed");
