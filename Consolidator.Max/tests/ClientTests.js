var assert = require("assert");
var fs = require("fs");
var path = require("path");
var vm = require("vm");

var root = path.resolve(__dirname, "..");
[
    "js/Clients/NativeProtocolClient.js",
    "js/Clients/StateClient.js",
    "js/Clients/AnalysisClient.js",
    "js/Clients/ConsolidatorClient.js"
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

testStateRoundTrip();
testAnalysisViewFiltering();
console.log("ClientTests passed");
