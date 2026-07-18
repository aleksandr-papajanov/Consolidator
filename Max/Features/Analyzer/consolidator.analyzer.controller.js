autowatch = 1;
inlets = 2;
outlets = 1;

include("../Shared/JS/DictionaryReader.js");
include("../Shared/JS/Messages/MessageEnvelope.js");
include("../Shared/JS/Messages/MessageFactory.js");

// Inlet 0: local SpectrumView envelopes: message <dictionary>.
// Inlet 1: direct Analyzer status and diagnostics: status <state> or error <code>.
// Outlet 0: interfeature envelopes: message <dictionary> to BusHub.

function AnalyzerFeatureController() {
    this.lastStatus = "initializing";
    this.lastError = "";
}

AnalyzerFeatureController.prototype.HandleStatus = function(command, values) {
    if (command === "status" && values.length > 0) {
        this.lastStatus = String(values[0]);
    }
    else if (command === "error" && values.length > 0) {
        this.lastError = String(values[0]);
    }
};

var controller = new AnalyzerFeatureController();

function inletassist(index) {
    assist(index === 0
        ? "Local SpectrumView envelope: message <dictionary>"
        : "Direct Analyzer status: status <state> or error <code>");
}

function outletassist(index) {
    assist(index === 0
        ? "Interfeature envelope: message <dictionary> to BusHub"
        : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function message() {
    var values = arrayfromargs(arguments);
    if (inlet === 0 && values.length === 1) {
        outlet(0, "message", values[0]);
    }
}

function loadbang() {
    var message = MessageFactory.create(
        "system.status", "bus.hub", { feature: "analyzer", state: "ready" }, "analyzer");
    var dictionary = MessageFactory.toMax(message);
    if (dictionary) outlet(0, "message", dictionary.name);
}

function anything() {
    if (inlet === 1) {
        controller.HandleStatus(messagename, arrayfromargs(arguments));
    }
}
