autowatch = 1;
inlets = 1;
outlets = 1;

function inletassist(index) {
    assist(index === 0 ? "DSP processor direct status: status ready or error <code>" : "");
}

function outletassist(index) {
    assist(index === 0 ? "Diagnostics: error <code>" : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function status() {}
function error(code) { outlet(0, "error", code); }
