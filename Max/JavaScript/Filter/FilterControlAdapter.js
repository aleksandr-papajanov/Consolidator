autowatch = 1;
inlets = 1;
outlets = 1;

include("../Messages/MessageEnvelope.js");
include("ControlAdapter.js");

var adapter = new ControlAdapter({
    gain: "gain_dial",
    frequency: "freq.numbox",
    q: "q.numbox",
    bypass: "bypass",
    reset: "reset"
});

function message() {
    var envelope = MessageEnvelope.fromMaxDictionary(arrayfromargs(arguments));
    if (!envelope || envelope.type !== "filter.control") {
        return;
    }

    var command = adapter.createCommand({
        controlId: envelope.payloadValue("control"),
        action: envelope.payloadValue("action"),
        values: envelope.payloadValue("values") || []
    });
    if (command) {
        outlet(0, command);
    }
}
