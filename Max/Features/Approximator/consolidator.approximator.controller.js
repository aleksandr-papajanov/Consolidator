autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: local commands fit, listen 0|1, clear.
// Inlet 1: native status status initialized|idle|ready|processing|error <code>.
// Outlet 0: Host atom commands: component.attach, analyzer.listen, fit.start, fit.clear.
// Outlet 1: status forwarded to the local patch.
// Outlet 2: thispatcher commands for Fit and Listen controls.

function ApproximatorFeatureController() {
    this.requestId = 0;
    this.listenEnabled = false;
    this.fitting = false;
    this.ready = false;
    this.hostReady = false;
}

ApproximatorFeatureController.prototype.SendCommand = function(name, values) {
    this.requestId += 1;
    outlet(0, "command", [1, "approximator.ui", this.requestId, name]
        .concat(values || []));
};

ApproximatorFeatureController.prototype.HandleLocalCommand = function(command, values) {
    if (command === "fit") {
        if (!this.listenEnabled || this.fitting) return;
        outlet(2, "script", "sendbox", "fit_button", "set", 0);
        this.SendCommand("fit.start", []);
        return;
    }
    if (command === "clear") {
        if (this.hostReady) this.SendCommand("fit.clear", []);
        return;
    }
    if (command === "listen" && values.length === 1) {
        this.listenEnabled = Number(values[0]) !== 0;
        if (!this.listenEnabled) this.fitting = false;
        if (this.hostReady) {
            this.SendCommand("analyzer.listen", [this.listenEnabled ? 1 : 0]);
            if (!this.listenEnabled) this.SendCommand("fit.clear", []);
        }
        this.UpdateControls();
    }
};

ApproximatorFeatureController.prototype.HandleNativeStatus = function(state, values) {
    if (state === "initialized") {
        this.hostReady = true;
        this.SendCommand("analyzer.listen", [this.listenEnabled ? 1 : 0]);
    }
    else if (state === "ready") {
        this.ready = true;
        this.fitting = false;
    }
    else if (state === "processing") {
        this.ready = false;
        this.fitting = true;
    }
    else if (state === "idle" || state === "error") {
        this.ready = false;
        this.fitting = false;
    }
    this.UpdateControls();
    outlet(1, "status", [state].concat(values || []));
};

ApproximatorFeatureController.prototype.UpdateControls = function() {
    outlet(2, "script", "sendbox", "fit_button", "active",
        this.listenEnabled && this.ready && !this.fitting ? 1 : 0);
    outlet(2, "script", "sendbox", "listen_button", "active",
        this.fitting ? 0 : 1);
};

var controller = new ApproximatorFeatureController();

function inletassist(index) {
    assist(index === 0
        ? "Commands: fit, listen 0|1, clear"
        : "Native status: status initialized|idle|ready|processing|error <code>");
}

function outletassist(index) {
    var descriptions = [
        "Host commands: component.attach, analyzer.listen, fit.start, fit.clear",
        "Local status: status <state>",
        "thispatcher commands for Fit and Listen controls"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() {
    controller.UpdateControls();
    controller.SendCommand("component.attach", [11, "approximator"]);
    outlet(2, "script", "sendbox", "listen_button", "outputvalue");
}

function fit() { if (inlet === 0) controller.HandleLocalCommand("fit", []); }
function listen(value) { if (inlet === 0) controller.HandleLocalCommand("listen", [value]); }
function clear() { if (inlet === 0) controller.HandleLocalCommand("clear", []); }

function status(state) {
    if (inlet === 1) controller.HandleNativeStatus(state, arrayfromargs(arguments).slice(1));
}

function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) controller.HandleLocalCommand(messagename, values);
    else controller.HandleNativeStatus(messagename, values);
}
