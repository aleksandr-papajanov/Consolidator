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
  client.handleControl("initialized", [1, "17", "1", "17"]);
  assert.strictEqual(String(result.instanceId), "17");
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
  client.uiTarget.show("4", 2);
  assert.deepStrictEqual(frames[0], ["observe_target", 1, "ui", "1", "4", 2]);
  client.handleControl("target_state_begin", [1, "9", "1", "4", 2, 1]);
  client.handleControl("target_state_entry", [
    1, "9", "1", 0, "compressor.threshold", -24, "ready",
    -60, 0, -60, 0,
  ]);
  client.handleControl("target_state_done", [1, "9", "1", "4", 2, 1]);
  client.handleControl("state_changed", [
    1, "compressor.threshold", -18, "ready", -60, 0, -60, 0,
  ]);
  assert.deepStrictEqual(values, [-24, -18]);
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
    "write", 1, "ui", "1", "8", "0", 1,
    "entry", "equalizer", "bank", 3, "filter", 2, "gain", "value", 4.5,
  ]);
}

testInitializationUsesExternalIdentityFromManaged();
testTargetSnapshotAndPushUpdateUseSemanticPaths();
testObservedEqualizerPathsAreExpandedForWrites();
console.log("UiProtocolTests passed");
