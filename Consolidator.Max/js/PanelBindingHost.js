inlets = 1;
outlets = 0;

function route(controlId, args) {
    if (args.length === 0) {
        return;
    }
    if (!patcher || typeof patcher.getnamed !== "function") {
        return;
    }
    var control = patcher.getnamed(controlId);
    if (!control || typeof control.message !== "function") {
        return;
    }
    control.message.apply(control, args);
}

function anything() {
    route(String(messagename), arrayfromargs(arguments));
}

function list() {
    var args = arrayfromargs(arguments);
    if (args.length === 0) {
        return;
    }
    route(String(args.shift()), args);
}
