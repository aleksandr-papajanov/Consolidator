autowatch = 1;
inlets = 2;
outlets = 1;

var requestId = 0;

// Analyzer has no domain state in JS. It forwards SpectrumView Host commands
// and keeps the direct native status available for future UI controls.
function inletassist(index) {
    assist(index === 0
        ? "Spectrum commands: command 1 spectrum <requestId> eq.set_parameter ..."
        : "Analyzer status: status ready|processing|error <code>");
}

function outletassist(index) {
    assist(index === 0 ? "Host commands: command 1 spectrum ..." : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() {
    requestId += 1;
    outlet(0, "command", 1, "analyzer.ui", requestId, "component.attach", 10, "analyzer");
}

function anything() {
    if (inlet === 0) outlet(0, messagename, arrayfromargs(arguments));
}
