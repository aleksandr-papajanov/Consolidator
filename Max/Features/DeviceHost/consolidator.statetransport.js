autowatch = 1;
inlets = 1;
outlets = 7;

function StateTransport() {}

StateTransport.prototype.RouteEvent = function(values) {
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
    } else if (store === "definitions" ||
        store === "processor_definitions") {
        outlet(3, "snapshot", values);
    } else if (store === "device") {
        outlet(4, "snapshot", values);
    } else if (store === "processor") {
        outlet(5, "snapshot", values);
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
    }
}

function inletassist() {
    assist("Host output: event or snapshot atom message");
}

function outletassist(index) {
    assist([
        "Runtime events",
        "EQ state snapshots",
        "DSP state snapshots",
        "Filter and processor definition snapshots",
        "Device identity snapshots",
        "Processor state snapshots",
        "Analyzer events and EQ snapshots"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
