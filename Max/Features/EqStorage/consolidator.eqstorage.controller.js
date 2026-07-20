autowatch = 1;
inlets = 3;
outlets = 2;

// Inlet 0: local UI commands: initialize, bang, add, remove, select, rename.
// Inlet 1: direct EqStorage status: status <state> or error <code>.
// Inlet 2: bank list commands: clear, append <name> <bankId>, setid <bankId>.
// Outlet 0: local EqStorage commands: initialize, bang, add, remove, select, rename.
// Outlet 1: bank list commands: clear, append, setid.

function EqStorageFeatureController() {
    this.lastStatus = "initializing";
    this.lastError = "";
}

EqStorageFeatureController.prototype.ForwardCommand = function(command, args) {
    if (args.length === 0) outlet(0, command);
    else if (args.length === 1) outlet(0, command, args[0]);
    else outlet(0, command, args[0], args[1]);
};

EqStorageFeatureController.prototype.HandleStatus = function(command, args) {
    if (command === "status" && args.length > 0) {
        this.lastStatus = String(args[0]);
    }
    else if (command === "error" && args.length > 0) {
        this.lastError = String(args[0]);
        post("EqStorage: " + this.lastError + "\n");
    }
};

EqStorageFeatureController.prototype.ForwardListCommand = function(command, args) {
    if (args.length === 0) outlet(1, command);
    else if (args.length === 1) outlet(1, command, args[0]);
    else outlet(1, command, args[0], args[1]);
};

var controller = new EqStorageFeatureController();

function inletassist(index) {
    assist(index === 0
        ? "Local UI commands: initialize, bang, add, remove, select, rename"
        : index === 1
            ? "Direct EqStorage status: status <state> or error <code>"
            : "Bank list commands: clear, append <name> <bankId>, setid <bankId>");
}

function outletassist(index) {
    assist(index === 0
        ? "Local EqStorage commands: initialize, bang, add, remove, select, rename"
        : "Bank list commands: clear, append <name> <bankId>, setid <bankId>");
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
