autowatch = 1;
inlets = 2;
outlets = 3;

// Inlets: 0 local commands fit, listen 0|1, clear;
// 1 native ready 0|1, fit_started, fit_finished, loss <value>, error <code>.
// Outlets: 0 envelopes; 1 status <state> [values]; 2 thispatcher script commands.

include("../Shared/JS/DictionaryReader.js");
include("../Shared/JS/Messages/MessageEnvelope.js");
include("../Shared/JS/Messages/MessageFactory.js");

function ApproximatorFeatureController() {
    this.featureId = "approximator";
    this.sourceId = "approximator.ui";
    this.listenEnabled = false;
    this.ready = false;
    this.fitting = false;
}

ApproximatorFeatureController.prototype.EmitEnvelope = function(type, target, payload) {
    var message = MessageFactory.create(type, target, payload || {}, this.sourceId);
    var dictionary = MessageFactory.toMax(message);
    if (dictionary) {
        outlet(0, "message", dictionary.name);
    }
};

ApproximatorFeatureController.prototype.HandleLocalCommand = function(command, values) {
    if (command === "fit") {
        if (!this.listenEnabled || this.fitting) {
            return;
        }

        outlet(2, "script", "sendbox", "fit_button", "set", 0);
        this.EmitEnvelope("approximator.fit", this.featureId, {});
        return;
    }

    if (command === "clear") {
        this.EmitEnvelope("approximator.clear", this.featureId, {});
        return;
    }

    if (command === "listen" && values.length === 1) {
        var enabled = Number(values[0]) !== 0 ? 1 : 0;
        this.listenEnabled = enabled !== 0;
        if (!this.listenEnabled) {
            this.ready = false;
            this.fitting = false;
        }
        this.UpdateControls();

        this.EmitEnvelope("analyzer.difference", "analyzer", {
            value: enabled
        });

        if (!enabled) {
            this.EmitEnvelope("approximator.clear", this.featureId, {});
        }
    }
};

ApproximatorFeatureController.prototype.HandleNativeStatus = function(state, values) {
    if (state === "ready" && values.length > 0) {
        this.ready = Number(values[0]) !== 0;
    }
    else if (state === "fit_started") this.fitting = true;
    else if (state === "fit_finished" || state === "error") this.fitting = false;

    this.UpdateControls();

    outlet(1, ["status", state].concat(values));
};

ApproximatorFeatureController.prototype.UpdateControls = function() {
    var fitActive = this.listenEnabled && !this.fitting ? 1 : 0;
    var listenActive = this.fitting ? 0 : 1;
    outlet(2, "script", "sendbox", "fit_button", "active", fitActive);
    outlet(2, "script", "sendbox", "listen_button", "active", listenActive);
};

var controller = new ApproximatorFeatureController();

function inletassist(index) {
    var descriptions = [
        "Commands: fit, listen 0|1, clear",
        "Native events: ready 0|1, fit_started, fit_finished, loss <value>, error <code>"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    var descriptions = [
        "message <envelope dictionary> to the message bus",
        "Feature status: status <state> [values]",
        "thispatcher commands for Fit and Listen controls"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() {
    controller.UpdateControls();
    outlet(2, "script", "sendbox", "listen_button", "outputvalue");
    controller.EmitEnvelope("system.status", "bus.hub", {
        feature: "approximator",
        state: "ready"
    });
}

function fit() {
    if (inlet === 0) controller.HandleLocalCommand("fit", []);
}

function listen(value) {
    if (inlet === 0) controller.HandleLocalCommand("listen", [value]);
}

function clear() {
    if (inlet === 0) controller.HandleLocalCommand("clear", []);
}

function status() {
    if (inlet === 1) {
        var values = arrayfromargs(arguments);
        controller.HandleNativeStatus(values[0], values.slice(1));
    }
}

function ready(value) {
    if (inlet === 1) {
        controller.HandleNativeStatus("ready", [value]);
    }
}

function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) {
        controller.HandleLocalCommand(messagename, values);
        return;
    }
    if (inlet === 1) {
        controller.HandleNativeStatus(messagename, values);
    }
}
