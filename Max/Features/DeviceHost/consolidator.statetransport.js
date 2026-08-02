autowatch = 1;
inlets = 2;
outlets = 12;

function StateTransport() {}

StateTransport.prototype.RouteEvent = function(values) {
    if (String(values[3]) === "parameter.updated") {
        outlet(7, "event", values);
        if (String(values[5]) === "eq") outlet(6, "event", values);
        return;
    }
    outlet(0, "event", values);
    outlet(6, "event", values);
};

StateTransport.prototype.RouteSnapshot = function(values) {
    if (values.length < 4 || Number(values[0]) !== 1 ||
        String(values[1]) !== "host") return;
    var store = String(values[2]);
    if (store === "eq") {
        outlet(1, "snapshot", values);
        outlet(6, "snapshot", values);
    } else if (store === "dsp") {
        outlet(2, "snapshot", values);
    } else if (store === "device") {
        outlet(4, "snapshot", values);
    } else if (store === "processor") {
        outlet(5, "snapshot", values);
    }
};

StateTransport.prototype.RouteAnalyzerUiState = function(name, values) {
    if (String(name) === "eq_preview" && values.length === 4) {
        outlet(8, "eq_preview", values);
    } else if (String(name) === "filter_limits" && values.length === 5) {
        outlet(8, "filter_limits", values);
    }
};

StateTransport.prototype.RouteCoordinatorState = function(name, values) {
    if (String(name) === "eq_preview" && values.length === 4) {
        outlet(8, "eq_preview", values);
    } else if (String(name) === "coordinator_directory") {
        outlet(9, "coordinator_directory", values);
    } else if (String(name) === "coordinator_processor_limits") {
        outlet(10, "processor_limits", values);
    } else if (String(name) === "coordinator_processor_preview") {
        outlet(10, "processor_preview", values);
    } else if (String(name) === "coordinator_filter_limits") {
        outlet(11, "filter_limits", values);
    }
};

var stateTransport = new StateTransport();

function event() {
    stateTransport.RouteEvent(arrayfromargs(arguments));
}

function snapshot() {
    stateTransport.RouteSnapshot(arrayfromargs(arguments));
}

function list() {
    var values = arrayfromargs(arguments);
    if (!values.length) return;
    var category = String(values[0]);
    if (category === "event") stateTransport.RouteEvent(values.slice(1));
    else if (category === "snapshot") {
        stateTransport.RouteSnapshot(values.slice(1));
    } else stateTransport.RouteCoordinatorState(category, values.slice(1));
}

function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 1) {
        stateTransport.RouteAnalyzerUiState(messagename, values);
    } else stateTransport.RouteCoordinatorState(messagename, values);
}

function inletassist(index) {
    assist(index === 0
        ? "Host output: event or snapshot atom message"
        : "Analyzer UI state: eq_preview <bankId> <filterId> <parameterIndex> <absoluteValue>; filter_limits <bankId> <filterId> <parameterIndex> <minimum> <maximum>");
}

function outletassist(index) {
    assist([
        "Runtime events",
        "EQ state snapshots",
        "DSP state snapshots",
        "Unused",
        "Device identity snapshots",
        "Processor state snapshots",
        "Analyzer events and EQ snapshots",
        "Continuous Host parameter events for DSP",
        "Analyzer UI: eq_preview and filter_limits"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
