autowatch = 1;
inlets = 1;
outlets = 1;

// Inlet 0: message <envelope-dictionary> from any feature outlet.
// Outlet 0: message <envelope-dictionary> broadcast to all feature inlets.
function BusHub() {
}

BusHub.prototype.ForwardEnvelope = function(dictionaryName) {
    outlet(0, "message", String(dictionaryName));
};

var busHub = new BusHub();

function inletassist(index) {
    assist(index === 0
        ? "message <envelope dictionary> from a feature"
        : "");
}

function outletassist(index) {
    assist(index === 0
        ? "message <envelope dictionary> to all features"
        : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function message() {
    var values = arrayfromargs(arguments);
    if (values.length === 1) {
        busHub.ForwardEnvelope(values[0]);
    }
}
