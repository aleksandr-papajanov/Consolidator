autowatch = 1;
inlets = 2;
outlets = 3;

function ApproximatorFeatureController() {
    this.requestId = 0;
    this.fitCurve = [];
    this.fitting = false;
    this.nativeReady = false;
    this.listenEnabled = false;
}

ApproximatorFeatureController.prototype.SendFit = function() {
    if (this.fitting || !this.nativeReady || this.fitCurve.length < 2) return;
    this.requestId += 1;
    this.fitting = true;
    this.UpdateControl();
    outlet(0, "command", [1, "approximator.ui", this.requestId, "fit.start", this.fitCurve.length]
        .concat(this.fitCurve));
};

ApproximatorFeatureController.prototype.SetFitCurve = function(values) {
    if (!values || values.length < 3 || String(values[0]) !== "fit_curve") return;
    var curve = values.slice(1).map(Number);
    if (!curve.every(isFinite)) return;
    this.fitCurve = curve;
    this.UpdateControl();
};

ApproximatorFeatureController.prototype.SetListen = function(value) {
    this.listenEnabled = Number(value) !== 0;
    this.requestId += 1;
    outlet(0, "command", [1, "approximator.ui", this.requestId, "analyzer.listen", this.listenEnabled ? 1 : 0]);
    this.UpdateControl();
};

ApproximatorFeatureController.prototype.SetStatus = function(state, values) {
    if (state === "ready") this.nativeReady = Number(values[0]) !== 0;
    if (state === "processing") this.fitting = true;
    if (state === "idle" || state === "ready" || state === "error") this.fitting = false;
    this.UpdateControl();
    outlet(1, "status", [state].concat(values || []));
};

ApproximatorFeatureController.prototype.UpdateControl = function() {
    outlet(2, "script", "sendbox", "approximator.match", "loadingIndex", this.fitting ? 1 : 0);
    outlet(2, "script", "sendbox", "approximator.match", "enabled",
        !this.fitting && this.nativeReady && this.fitCurve.length > 1 ? 1 : 0);
    outlet(2, "script", "sendbox", "approximator.listen", "enabled", !this.fitting ? 1 : 0);
    outlet(2, "script", "sendbox", "approximator.listen", "set", this.listenEnabled ? 1 : 0);
};

var controller = new ApproximatorFeatureController();

function inletassist(index) {
    assist(index === 0 ? "match 1 <0|1>; listen <0|1>" : "fit_curve <dB...> or native status");
}

function outletassist(index) {
    assist([
        "command 1 approximator.ui <id> fit.start <pointCount> <curve...>; analyzer.listen <0|1>",
        "status <state>",
        "thispatcher commands for Match EQ"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function match(index, value) {
    if (inlet === 0 && Number(index) === 1 && Number(value) !== 0) controller.SendFit();
}
function listen(value) {
    if (inlet === 0) controller.SetListen(value);
}
function status(state) {
    if (inlet === 1) controller.SetStatus(state, arrayfromargs(arguments).slice(1));
}
function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 1 && messagename === "fit_curve") controller.SetFitCurve(["fit_curve"].concat(values));
    else if (inlet === 1) controller.SetStatus(messagename, values);
}
function list() {
    if (inlet === 1) controller.SetFitCurve(arrayfromargs(arguments));
}
