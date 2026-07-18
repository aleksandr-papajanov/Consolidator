autowatch = 1;
inlets = 1;
outlets = 1;

include("../Shared/JS/DictionaryReader.js");
include("../Shared/JS/Messages/MessageEnvelope.js");
include("../Shared/JS/Messages/MessageFactory.js");

// Inlet 0: message <envelope-dictionary> from any feature outlet.
// Outlet 0: message <envelope-dictionary> broadcast to all feature inlets.
function BusHub() {
    this.requiredFeatures = ["analyzer", "approximator", "eq.storage"];
    this.readyFeatures = {};
    this.started = false;
}

BusHub.prototype.ForwardEnvelope = function(dictionaryName) {
    var message = MessageFactory.fromMax(dictionaryName);
    if (!message) {
        post("BusHub: invalid_message_envelope\n");
        return;
    }
    outlet(0, "message", String(dictionaryName));
    if (message.type !== "system.status") {
        return;
    }

    var feature = String(message.payload.feature || message.source || "");
    var state = String(message.payload.state || "");
    if (feature.length > 0) {
        this.readyFeatures[feature] = state === "ready";
    }
    this.StartWhenReady();
};

BusHub.prototype.StartWhenReady = function() {
    if (this.started) return;
    for (var index = 0; index < this.requiredFeatures.length; index++) {
        if (!this.readyFeatures[this.requiredFeatures[index]]) return;
    }
    this.started = true;
    var start = MessageFactory.create("system.start", "broadcast", {}, "bus.hub");
    var dictionary = MessageFactory.toMax(start);
    if (dictionary) outlet(0, "message", dictionary.name);
};

var busHub = new BusHub();

function inletassist(index) {
    assist(index === 0
        ? "message <envelope dictionary>; consumes system.status"
        : "");
}

function outletassist(index) {
    assist(index === 0
        ? "message <envelope dictionary>; produces system.start"
        : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function message() {
    var values = arrayfromargs(arguments);
    if (values.length === 1) {
        busHub.ForwardEnvelope(values[0]);
    }
}
