autowatch = 1;
inlets = 1;
outlets = 1;

include("../Messages/MessageEnvelope.js");

var slot = Number(jsarguments[1]);

function dictionary(name) {
    var configuration;
    try {
        configuration = new Dict(String(name));
    }
    catch (error) {
        errorMessage("invalid_filter_configuration_dictionary");
        return;
    }

    sendEnvelope("filter.define", { contractName: configuration.name });
}

function update(control, value) {
    var payload = {
        control: String(control),
        value: Number(value)
    };
    sendEnvelope("filter.control.update", payload);
}

function instance_state(recovered) {
    sendEnvelope("filter.instance.state", { recovered: Number(recovered) });
}

function reset() {
    sendEnvelope("filter.reset", {});
}

function sendEnvelope(type, payload) {
    var envelope = MessageEnvelope.create(type, slot, payload, "filter.ui");
    send(envelope.toMaxDictionary());
}

function send(message) {
    outlet(0, "message", message.name);
}

function errorMessage(code) {
    post("FilterCommandBridge: " + code + "\n");
}
