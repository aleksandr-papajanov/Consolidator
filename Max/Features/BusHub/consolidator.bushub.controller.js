autowatch = 1;
inlets = 1;
outlets = 1;

// Inlet 0: message <envelope dictionary> from ---message.bus.in.
// Outlet 0: message <envelope dictionary> to BusHub.js.

function inletassist(index) {
    assist(index === 0
        ? "message <envelope dictionary> from ---message.bus.in"
        : "");
}

function outletassist(index) {
    assist(index === 0
        ? "message <envelope dictionary> to BusHub.js"
        : "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function message() {
    var values = arrayfromargs(arguments);
    if (values.length === 1) outlet(0, "message", values[0]);
}
