autowatch = 1;
inlets = 4;
outlets = 4;

// Inlet 0: local source selection from the source menu.
// Inlet 1: local channel selection from the channel menu.
// Inlet 2: source menu update from Routing.maxpat.
// Inlet 3: channel menu update from Routing.maxpat.
// Outlet 0: source selection to Routing.maxpat.
// Outlet 1: channel selection to Routing.maxpat.
// Outlet 2: source menu update.
// Outlet 3: channel menu update.

function inletassist(index) {
    var descriptions = [
        "Local source selection",
        "Local channel selection",
        "Source menu update from Routing.maxpat",
        "Channel menu update from Routing.maxpat"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    var descriptions = [
        "Source selection to Routing.maxpat",
        "Channel selection to Routing.maxpat",
        "Source menu update",
        "Channel menu update"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function Forward(command, values) {
    var output = inlet;
    if (values.length === 0) outlet(output, command);
    else if (values.length === 1) outlet(output, command, values[0]);
    else outlet(output, command, values[0], values[1]);
}

function msg_int(value) {
    Forward(value, []);
}

function list() {
    Forward("list", arrayfromargs(arguments));
}

function anything() {
    Forward(messagename, arrayfromargs(arguments));
}
