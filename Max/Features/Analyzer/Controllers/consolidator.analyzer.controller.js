autowatch = 1;
inlets = 3;
outlets = 3;
include("../../../Shared/Runtime/LiveApiInitializer.js");
include("../../../Shared/Runtime/ControlControllerBase.js");

function AnalyzerController() {
    ControlControllerBase.call(this, "analyzer", this.FlushEqParameterCommand, this);
    this.mode = "spectrum";
    this.visible = false;
    this.viewPublished = false;
    this.hostReady = false;
    this.deviceId = 0;
    this.trackId = 0;
    this.selectedTrackId = 0;
    this.songView = null;
    this.initializer = new LiveApiInitializer(
        this.TryInitialize, this, 50);
}

AnalyzerController.prototype = Object.create(ControlControllerBase.prototype);
AnalyzerController.prototype.constructor = AnalyzerController;

AnalyzerController.prototype.ForwardCommand = function(name, values) {
    var commandValues = name === "command" ? values
        : name === "list" && String(values[0]) === "command"
            ? values.slice(1) : null;
    if (commandValues) {
        var commandName = String(commandValues[3]);
        if (commandName === "history.end") this.FlushPendingParameters();
        if (commandName === "eq.set_parameter" && commandValues.length === 8) {
            outlet(2, "eq_parameter_absolute_preview",
                Number(commandValues[4]), Number(commandValues[5]),
                String(commandValues[6]), Number(commandValues[7]));
            this.parameterDispatcher.Enqueue(
                [commandValues[4], commandValues[5], commandValues[6]].join(":"),
                commandValues.slice(0)
            );
            return;
        }
        if (commandName === "eq.reset_filter" && commandValues.length === 6) {
            outlet(2, "eq_filter_reset", Number(commandValues[4]), Number(commandValues[5]));
            return;
        }
        if (commandName === "eq.set_bypass" && commandValues.length >= 6) {
            outlet(0, "command", commandValues);
            return;
        }
        outlet(0, "command", commandValues);
        return;
    }
    if (name === "bank.action") {
        outlet(2, "bank.action", values);
        return;
    }
    outlet(0, name, values);
};

AnalyzerController.prototype.FlushEqParameterCommand = function(values) {
    outlet(2, "eq_parameter_absolute_gesture",
        Number(values[4]), Number(values[5]),
        String(values[6]), Number(values[7]));
    outlet(0, "command", values);
};

AnalyzerController.prototype.SetMode = function(value) {
    var mode = value === "analysis" || Number(value) === 1 ? "analysis" : "spectrum";
    if (this.mode === mode) return;
    this.mode = mode;
    outlet(1, "mode", this.mode);
    this.PublishViewState();
};

AnalyzerController.prototype.Initialize = function() {
    this.initializer.Start();
};

AnalyzerController.prototype.Dispose = function() {
    this.initializer.Dispose();
};

AnalyzerController.prototype.TryInitialize = function() {
    var device = new LiveAPI("this_device");
    var deviceId = Number(device.id);
    if (!isFinite(deviceId) || deviceId <= 0) return false;
    var trackId = this.ReadLiveId(device.get("canonical_parent"));
    if (!isFinite(trackId) || trackId <= 0) return false;
    var songView = new LiveAPI(
        AnalyzerSelectedTrackChanged, "live_set view");
    this.deviceId = deviceId;
    this.trackId = trackId;
    this.songView = songView;
    songView.property = "selected_track";
    this.ReadSelectedTrack(songView.get("selected_track"));
    return true;
};

AnalyzerController.prototype.ReadSelectedTrack = function(values) {
    this.selectedTrackId = this.ReadLiveId(values);
    var visible = this.selectedTrackId === this.trackId;
    if (this.viewPublished && this.visible === visible) return;
    this.visible = visible;
    this.viewPublished = true;
    this.PublishViewState();
};

AnalyzerController.prototype.ReadLiveId = function(values) {
    if (!values) return 0;
    for (var index = 0; index < values.length; ++index) {
        if (String(values[index]) === "id" && index + 1 < values.length) return Number(values[index + 1]);
    }
    return values.length ? Number(values[values.length - 1]) : 0;
};

AnalyzerController.prototype.PublishViewState = function() {
    if (!this.hostReady || this.deviceId <= 0) return;
    this.SendCommand("analyzer.set_view", [this.visible ? 1 : 0, this.mode]);
};

AnalyzerController.prototype.HandleStatus = function(name) {
    if (name !== "host_ready" || this.hostReady) return;
    this.hostReady = true;
    this.PublishViewState();
};

var analyzerController = new AnalyzerController();

function AnalyzerSelectedTrackChanged() {
    var values = arguments.length === 1 &&
        arguments[0] instanceof Array
        ? arguments[0]
        : arrayfromargs(arguments);
    analyzerController.ReadSelectedTrack(values);
}

function list() {
    if (inlet === 0) analyzerController.ForwardCommand("list", arrayfromargs(arguments));
}

function anything() {
    if (inlet === 0) analyzerController.ForwardCommand(messagename, arrayfromargs(arguments));
}

function status(name) {
    if (inlet === 1) analyzerController.HandleStatus(String(name));
}

function msg_int(value) {
    if (inlet === 0) analyzerController.SetMode(value);
}

function mode(value) {
    if (inlet === 0) analyzerController.SetMode(value);
}

function view_mode(value) {
    if (inlet === 0) analyzerController.SetMode(value);
}

function initialize() {
    if (inlet === 2) analyzerController.Initialize();
}

function loadbang() {
    analyzerController.Initialize();
}

function notifydeleted() {
    analyzerController.Dispose();
}

function inletassist(index) {
    var descriptions = [
        "Spectrum commands and analyzer view mode 0 spectrum, 1 analysis",
        "Analyzer status: status ready|host_ready",
        "Optional idempotent initialize trigger"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    assist(index === 0
        ? "command 1 analyzer <requestId> analyzer.set_view <visible> <spectrum|analysis>; fit.start <pointCount> <curve...>; analyzer.clear; history.begin <operationId>, history.end <operationId>; eq.set_parameter, eq.set_bypass, eq.reset_filter; bank.action <join|commit|reset|bypass> <optional 0|1>"
        : (index === 1
            ? "mode spectrum|analysis for the unified Analyzer View"
            : "eq_parameter_absolute_preview|eq_parameter_absolute_gesture <bankId> <filterId> <parameter> <absoluteValue>; eq_filter_reset <bankId> <filterId>"));
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
