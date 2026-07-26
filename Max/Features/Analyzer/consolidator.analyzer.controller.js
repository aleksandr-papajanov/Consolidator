autowatch = 1;
inlets = 3;
outlets = 2;

function AnalyzerController() {
    this.requestId = 0;
    this.mode = "spectrum";
    this.visible = false;
    this.deviceId = 0;
    this.selectedDeviceId = 0;
    this.selectedDeviceView = null;
}

AnalyzerController.prototype.ForwardCommand = function(name, values) {
    outlet(0, name, values);
};

AnalyzerController.prototype.SetMode = function(value) {
    var mode = value === "analysis" || Number(value) === 1 ? "analysis" : "spectrum";
    if (this.mode === mode) return;
    this.mode = mode;
    outlet(1, "mode", this.mode);
    this.PublishViewState();
};

AnalyzerController.prototype.Initialize = function() {
    this.deviceId = Number(new LiveAPI("this_device").id);
    if (!isFinite(this.deviceId) || this.deviceId <= 0) return;
    this.selectedDeviceView = new LiveAPI(
        AnalyzerSelectedDeviceChanged,
        "live_set view selected_track view");
    this.selectedDeviceView.property = "selected_device";
    this.ReadSelectedDevice(this.selectedDeviceView.get("selected_device"));
};

AnalyzerController.prototype.ReadSelectedDevice = function(values) {
    this.selectedDeviceId = this.ReadLiveId(values);
    var visible = this.selectedDeviceId === this.deviceId;
    if (this.visible === visible) return;
    this.visible = visible;
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
    if (this.deviceId <= 0) return;
    this.requestId += 1;
    outlet(0, "command", 1, "analyzer", this.requestId, "analyzer.set_view", this.visible ? 1 : 0, this.mode);
};

var analyzerController = new AnalyzerController();

function AnalyzerSelectedDeviceChanged(values) {
    analyzerController.ReadSelectedDevice(values);
}

function list() {
    if (inlet === 0) analyzerController.ForwardCommand("list", arrayfromargs(arguments));
}

function anything() {
    if (inlet === 0) analyzerController.ForwardCommand(messagename, arrayfromargs(arguments));
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

function inletassist(index) {
    var descriptions = [
        "Spectrum commands and analyzer view mode 0 spectrum, 1 analysis",
        "Analyzer status",
        "initialize after live.thisdevice"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    assist(index === 0
        ? "command 1 analyzer <requestId> analyzer.set_view <visible> <spectrum|analysis>; spectrum edit commands"
        : "mode spectrum|analysis for the unified Analyzer View");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
