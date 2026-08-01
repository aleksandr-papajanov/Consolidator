autowatch = 1;
inlets = 2;
outlets = 2;

include("../../../Shared/Runtime/ControlControllerBase.js");

function HistoryController() {
    ControlControllerBase.call(this, "history.ui", null, this);
    this.canUndo = false;
    this.canRedo = false;
    this.awaitingRestore = false;
}

HistoryController.prototype = Object.create(ControlControllerBase.prototype);
HistoryController.prototype.constructor = HistoryController;

HistoryController.prototype.UpdateControls = function() {
    outlet(1, "script", "sendbox", "history.actions", "buttonEnabled", 1,
        this.canUndo && !this.awaitingRestore ? 1 : 0);
    outlet(1, "script", "sendbox", "history.actions", "buttonEnabled", 2,
        this.canRedo && !this.awaitingRestore ? 1 : 0);
};

HistoryController.prototype.HandleAction = function(index, value) {
    if (Number(value) === 0 || this.awaitingRestore) return;
    var command = "";
    if (Number(index) === 1 && this.canUndo) command = "history.undo";
    if (Number(index) === 2 && this.canRedo) command = "history.redo";
    if (!command) return;
    this.awaitingRestore = true;
    this.UpdateControls();
    this.SendCommand(command, []);
};

HistoryController.prototype.HandleEvent = function(values) {
    if (values.length !== 7 || String(values[0]) !== "event" ||
        Number(values[1]) !== 1 || String(values[2]) !== "host" ||
        String(values[4]) !== "history.changed") return;
    this.canUndo = Number(values[5]) !== 0;
    this.canRedo = Number(values[6]) !== 0;
    this.awaitingRestore = false;
    this.UpdateControls();
};

var controller = new HistoryController();

function loadbang() { controller.UpdateControls(); }
function notifydeleted() { controller.Dispose(); }

function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) controller.HandleAction(values[0], values[1]);
    else controller.HandleEvent(values);
}

function event() {
    if (inlet === 1) controller.HandleEvent(["event"].concat(arrayfromargs(arguments)));
}

function inletassist(index) {
    assist(index === 0
        ? "ButtonGroup action: <1|2> <0|1> for Undo or Redo"
        : "Host event history.changed <canUndo> <canRedo>");
}

function outletassist(index) {
    assist(index === 0
        ? "Host command history.undo or history.redo"
        : "thispatcher commands for history.actions");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
