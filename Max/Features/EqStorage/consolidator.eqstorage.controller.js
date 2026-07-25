autowatch = 1;
inlets = 3;
outlets = 3;

// Inlet 0: local UI commands: action <1..5> <0|1>, select <bankId>, joinselection <count> <bankIds...>.
// Inlet 1: direct EqStorage status: status <state> or error <code>.
// Inlet 2: bank list commands: clear, append <name> <bankId> <bypass> <solo>, setstate <activeId> <joinCount> <joinIds...>.
// Outlet 0: local EqStorage commands.
// Outlet 1: bank list commands.
// Outlet 2: thispatcher commands for EqStorage actions.

function EqStorageFeatureController() {
    this.lastStatus = "initializing";
    this.lastError = "";
}

EqStorageFeatureController.prototype.ForwardCommand = function(command, args) {
    outlet(0, [command].concat(args));
};

EqStorageFeatureController.prototype.HandleStatus = function(command, args) {
    if (command === "status" && args.length > 0) {
        this.lastStatus = String(args[0]);
    }
    else if (command === "error" && args.length > 0) {
        this.lastError = String(args[0]);
        post("EqStorage: " + this.lastError + "\n");
    }
    else if (command === "actionstate" && args.length === 2) {
        outlet(2, "script", "sendbox", "eqstorage.actions", "setvalue", 3, Number(args[0]));
        outlet(2, "script", "sendbox", "eqstorage.actions", "setvalue", 4, Number(args[1]));
    }
};

EqStorageFeatureController.prototype.ForwardListCommand = function(command, args) {
    outlet(1, [command].concat(args));
};

var controller = new EqStorageFeatureController();

function inletassist(index) {
    assist(index === 0
        ? "Local UI commands: action <1..5> <0|1>, select <bankId>, joinselection <count> <bankIds...>"
        : index === 1
            ? "Direct EqStorage status: status <state> or error <code>"
            : "Bank list commands: clear, append <name> <bankId> <bypass> <solo>, setstate <activeId> <joinCount> <joinIds...>");
}

function outletassist(index) {
    assist(index === 0
        ? "Local EqStorage commands"
        : index === 1
            ? "Bank list commands: clear, append, setstate."
            : "thispatcher action-button state commands.");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function msg_int(value) {
    if (inlet === 0) controller.ForwardCommand("select", [value]);
}

function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 0 && values.length > 0) {
        controller.ForwardCommand(String(values[0]), values.slice(1));
    }
}

function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) controller.ForwardCommand(messagename, values);
    else if (inlet === 1) controller.HandleStatus(messagename, values);
    else controller.ForwardListCommand(messagename, values);
}
