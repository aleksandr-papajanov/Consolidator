autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: UI commands add [name], remove, select <bankId>, rename <bankId> <name>.
// Inlet 1: Host EQ snapshots: snapshot 1 host eq ... .
// Outlet 0: bank list commands clear, append <name> <bankId>, setid <bankId>.
// Outlet 1: status/error diagnostics.
// Outlet 2: Host commands on the common atom bus.

function EqStoragePresentation() {
    this.requestId = 0;
    this.selectedBankId = 1;
}

EqStoragePresentation.prototype.SendCommand = function(name, values) {
    this.requestId += 1;
    outlet(2, "command", [1, "eqstorage.ui", this.requestId, name]
        .concat(values || []));
};

EqStoragePresentation.prototype.HandleUiCommand = function(command, values) {
    if (command === "add") this.SendCommand("eq.add_bank", values.length ? [String(values[0])] : []);
    else if (command === "remove") this.SendCommand("eq.remove_bank", [this.selectedBankId]);
    else if (command === "select" && values.length === 1) this.SendCommand("eq.select_bank", [Number(values[0])]);
    else if (command === "rename" && values.length >= 2) this.SendCommand("eq.rename_bank", [Number(values[0]), String(values[1])]);
    else outlet(1, "error", "invalid_bank_command");
};

EqStoragePresentation.prototype.HandleSnapshot = function(values) {
    if (values.length < 7 || String(values[0]) !== "snapshot" || Number(values[1]) !== 1 ||
        String(values[2]) !== "host" || String(values[3]) !== "eq") {
        outlet(1, "error", "invalid_eq_snapshot");
        return;
    }
    var selected = Number(values[5]);
    var bankCount = Number(values[6]);
    if (!isFinite(selected) || !isFinite(bankCount) || bankCount < 1) {
        outlet(1, "error", "invalid_eq_snapshot");
        return;
    }
    this.selectedBankId = selected;
    var position = 7;
    var banks = [];
    var selectedFound = false;
    for (var bankIndex = 0; bankIndex < bankCount; bankIndex++) {
        if (position + 4 >= values.length) return outlet(1, "error", "invalid_eq_snapshot");
        var bankId = Number(values[position++]);
        var name = String(values[position++]);
        position += 2;
        var filterCount = Number(values[position++]);
        if (!isFinite(bankId) || bankId < 1 || !isFinite(filterCount) || filterCount < 0 ||
            Math.floor(filterCount) !== filterCount) return outlet(1, "error", "invalid_eq_snapshot");
        if (bankId === selected) selectedFound = true;
        banks.push({ id: bankId, name: name });
        for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
            if (position + 2 >= values.length) return outlet(1, "error", "invalid_eq_snapshot");
            position += 2;
            var valueCount = Number(values[position++]);
            if (!isFinite(valueCount) || valueCount < 0 || Math.floor(valueCount) !== valueCount ||
                position + valueCount > values.length) return outlet(1, "error", "invalid_eq_snapshot");
            position += valueCount;
        }
    }
    if (!selectedFound || position !== values.length) return outlet(1, "error", "invalid_eq_snapshot");
    outlet(0, "clear");
    for (var displayIndex = banks.length - 1; displayIndex >= 0; displayIndex--) {
        outlet(0, "append", String(banks[displayIndex].id) + "  " + banks[displayIndex].name,
            banks[displayIndex].id);
    }
    outlet(0, "setid", selected);
    outlet(1, "status", "ready");
};

var storage = new EqStoragePresentation();

function inletassist(index) {
    var descriptions = [
        "UI: add, remove, select <id>, rename <id> <name>",
        "Host snapshots: snapshot 1 host eq ..."
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    var descriptions = [
        "Bank list: clear, append <name> <id>, setid <id>",
        "status ready or error <code>",
        "Host commands: eq.add_bank, eq.remove_bank, eq.select_bank, eq.rename_bank"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function add() { if (inlet === 0) storage.HandleUiCommand("add", arrayfromargs(arguments)); }
function remove() { if (inlet === 0) storage.HandleUiCommand("remove", []); }
function select(value) { if (inlet === 0) storage.HandleUiCommand("select", [value]); }
function rename() { if (inlet === 0) storage.HandleUiCommand("rename", arrayfromargs(arguments)); }
function snapshot() {
    if (inlet !== 1) return;
    var values = ["snapshot"].concat(arrayfromargs(arguments));
    if (values.length > 3 && String(values[3]) === "eq") storage.HandleSnapshot(values);
}
function list() {
    if (inlet !== 1) return;
    var values = arrayfromargs(arguments);
    if (values.length > 3 && String(values[0]) === "snapshot" && String(values[3]) === "eq") {
        storage.HandleSnapshot(values);
    }
}
function anything() {
    if (inlet === 0) storage.HandleUiCommand(messagename, arrayfromargs(arguments));
    else if (inlet === 1 && messagename === "snapshot") {
        storage.HandleSnapshot(["snapshot"].concat(arrayfromargs(arguments)));
    }
}
