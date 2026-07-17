autowatch = 1;
inlets = 1;
outlets = 1;

include("Messages/MessageEnvelope.js");

var feature = String(jsarguments[1]);
var commandTypes = {
    analyzer: {
        difference: "analyzer.difference",
        stats: "analyzer.stats"
    },
    approximator: {
        clear: "approximator.clear",
        fit: "approximator.fit"
    }
};

function anything() {
    Emit(messagename, arrayfromargs(arguments));
}

function Emit(command, values) {
    var type = commandTypes[feature] && commandTypes[feature][String(command)];
    if (!type) {
        post("CommandEnvelopeAdapter: unsupported command " + command + "\n");
        return;
    }

    var payload = {};
    if (values.length === 1) {
        payload.value = Number(values[0]);
    }

    var envelope = MessageEnvelope.create(type, feature, payload, feature + ".ui");
    var dictionary = envelope.toMaxDictionary();
    outlet(0, "message", dictionary.name);
}
