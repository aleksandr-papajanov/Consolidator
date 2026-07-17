autowatch = 1;
inlets = 1;
outlets = 1;

include("MessageEnvelope.js");

var commandTypes = {
    difference: "analyzer.difference",
    fit: "approximator.fit",
    clear: "approximator.clear",
    publish: "analyzer.publish",
    stats: "analyzer.stats"
};

function bang() {
    emit("publish", []);
}

function anything() {
    emit(messagename, arrayfromargs(arguments));
}

function emit(command, values) {
    var type = commandTypes[String(command)];
    if (!type) {
        post("EnvelopeEmitter: unsupported command " + command + "\n");
        return;
    }

    var payload = {};
    if (values.length === 1) {
        payload.value = Number(values[0]);
    }
    var envelope = MessageEnvelope.create(type, null, payload, "max.ui");
    var dictionary = envelope.toMaxDictionary();
    outlet(0, "message", dictionary.name);
}
