autowatch = 1;
inlets = 2;
outlets = 1;

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

function anything() {
    if (inlet === 0) outlet(0, messagename, arrayfromargs(arguments));
}
