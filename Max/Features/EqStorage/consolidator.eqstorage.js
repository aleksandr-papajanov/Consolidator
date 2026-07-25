autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: UI commands: action <1..5>, select <bankId>, joinselection <count> <bankIds...>.
// Inlet 1: Host EQ snapshots: snapshot 1 host eq ... .
// Outlet 0: bank-list commands: clear, append <name> <bankId> <bypass> <solo>, setstate <activeId> <joinCount> <joinIds...>.
// Outlet 1: status/error diagnostics.
// Outlet 2: Host commands: eq.add_bank, eq.remove_banks, eq.set_banks_bypass, eq.solo_banks, eq.join_banks, eq.select_bank.

function EqStoragePresentation() {
    this.requestId = 0;
    this.selectedBankId = 1;
    this.joinedBankIds = [];
    this.banks = [];
}

EqStoragePresentation.prototype.SendCommand = function(name, values) {
    this.requestId += 1;
    outlet(2, "command", [1, "eqstorage.ui", this.requestId, name].concat(values || []));
};

EqStoragePresentation.prototype.SelectedValues = function() {
    var bankIds = this.ActionBankIds();
    return [bankIds.length].concat(bankIds);
};

EqStoragePresentation.prototype.ActionBankIds = function() {
    return this.joinedBankIds.length > 0 ? this.joinedBankIds : [this.selectedBankId];
};

EqStoragePresentation.prototype.HandleSelect = function(value) {
    var bankId = Number(value);
    if (!isFinite(bankId) || !this.FindBank(bankId)) {
        return outlet(1, "error", "invalid_bank_selection");
    }
    this.selectedBankId = bankId;
    this.joinedBankIds = [];
    this.PublishActionState();
    this.SendCommand("eq.select_bank", [bankId]);
};

EqStoragePresentation.prototype.HandleJoinSelection = function(values) {
    if (values.length < 1) return outlet(1, "error", "invalid_bank_selection");
    var count = Number(values[0]);
    if (!isFinite(count) || count < 0 || values.length !== count + 1) {
        return outlet(1, "error", "invalid_bank_selection");
    }
    var ids = [];
    for (var index = 0; index < count; index++) {
        var bankId = Number(values[index + 1]);
        if (!isFinite(bankId) || bankId < 1 || ids.indexOf(bankId) >= 0 || !this.FindBank(bankId)) {
            return outlet(1, "error", "invalid_bank_selection");
        }
        ids.push(bankId);
    }
    this.joinedBankIds = ids;
    this.PublishActionState();
};

EqStoragePresentation.prototype.HandleAction = function(index, value) {
    var action = Number(index);
    if (Number(value) === 0 && action !== 3 && action !== 4) return;
    if (action === 1) this.SendCommand("eq.add_bank", []);
    else if (action === 2) this.SendCommand("eq.remove_banks", this.SelectedValues());
    else if (action === 3) this.SetSelectedBypass();
    else if (action === 4) this.SendCommand("eq.solo_banks", this.SelectedValues());
    else if (action === 5) this.SendCommand("eq.join_banks", this.SelectedValues());
    else outlet(1, "error", "invalid_bank_action");
};

EqStoragePresentation.prototype.SetSelectedBypass = function() {
    var bankIds = this.ActionBankIds();
    var shouldBypass = false;
    for (var index = 0; index < bankIds.length; index++) {
        var bank = this.FindBank(bankIds[index]);
        shouldBypass = shouldBypass || (bank && !bank.bypass);
    }
    this.SendCommand("eq.set_banks_bypass", [shouldBypass ? 1 : 0].concat(this.SelectedValues()));
};

EqStoragePresentation.prototype.FindBank = function(bankId) {
    for (var index = 0; index < this.banks.length; index++) {
        if (this.banks[index].id === bankId) return this.banks[index];
    }
    return null;
};

EqStoragePresentation.prototype.PublishActionState = function() {
    var bankIds = this.ActionBankIds();
    var bypass = bankIds.length > 0;
    var solo = bankIds.length > 0;
    for (var index = 0; index < bankIds.length; index++) {
        var bank = this.FindBank(bankIds[index]);
        bypass = bypass && bank && bank.bypass;
        solo = solo && bank && bank.solo;
    }
    outlet(1, "actionstate", bypass ? 1 : 0, solo ? 1 : 0);
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
    var position = 7;
    var banks = [];
    for (var bankIndex = 0; bankIndex < bankCount; bankIndex++) {
        if (position + 4 >= values.length) return outlet(1, "error", "invalid_eq_snapshot");
        var bankId = Number(values[position++]);
        var name = String(values[position++]);
        var bypass = Number(values[position++]) !== 0;
        var solo = Number(values[position++]) !== 0;
        var filterCount = Number(values[position++]);
        if (!isFinite(bankId) || bankId < 1 || !isFinite(filterCount) || filterCount < 0 ||
            Math.floor(filterCount) !== filterCount) return outlet(1, "error", "invalid_eq_snapshot");
        banks.push({ id: bankId, name: name, bypass: bypass, solo: solo });
        for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
            if (position + 2 >= values.length) return outlet(1, "error", "invalid_eq_snapshot");
            position += 2;
            var valueCount = Number(values[position++]);
            if (!isFinite(valueCount) || valueCount < 0 || Math.floor(valueCount) !== valueCount ||
                position + valueCount > values.length) return outlet(1, "error", "invalid_eq_snapshot");
            position += valueCount;
        }
    }
    if (position !== values.length) return outlet(1, "error", "invalid_eq_snapshot");

    this.banks = banks;
    var retainedIds = this.joinedBankIds.filter(function(bankId) {
        return banks.some(function(bank) { return bank.id === bankId; });
    });
    this.selectedBankId = selected;
    this.joinedBankIds = retainedIds;

    outlet(0, "clear");
    for (var displayIndex = banks.length - 1; displayIndex >= 0; displayIndex--) {
        var bank = banks[displayIndex];
        outlet(0, "append", String(bank.id) + "  " + bank.name, bank.id,
            bank.bypass ? 1 : 0, bank.solo ? 1 : 0);
    }
    outlet(0, ["setstate", this.selectedBankId, this.joinedBankIds.length]
        .concat(this.joinedBankIds));
    this.PublishActionState();
    outlet(1, "status", "ready");
};

var storage = new EqStoragePresentation();

function inletassist(index) {
    assist(index === 0
        ? "action <1 Add|2 Remove|3 Bypass|4 Solo|5 Join> <0|1>; select <bankId>; joinselection <count> <bankIds...>"
        : "Host snapshots: snapshot 1 host eq ...");
}

function outletassist(index) {
    var descriptions = [
        "Bank list: clear, append <name> <bankId> <bypass> <solo>, setstate <activeId> <joinCount> <joinIds...>",
        "status ready or error <code>",
        "Host commands: eq.add_bank, eq.remove_banks, eq.set_banks_bypass, eq.solo_banks, eq.join_banks, eq.select_bank"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function action(index, value) { if (inlet === 0) storage.HandleAction(index, value); }
function select(bankId) { if (inlet === 0) storage.HandleSelect(bankId); }
function joinselection() { if (inlet === 0) storage.HandleJoinSelection(arrayfromargs(arguments)); }
function snapshot() {
    if (inlet !== 1) return;
    storage.HandleSnapshot(["snapshot"].concat(arrayfromargs(arguments)));
}
function list() {
    if (inlet !== 1) return;
    var values = arrayfromargs(arguments);
    if (values.length > 3 && String(values[0]) === "snapshot" && String(values[3]) === "eq") {
        storage.HandleSnapshot(values);
    }
}
function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0 && messagename === "select") storage.HandleSelect(values[0]);
    else if (inlet === 0 && messagename === "joinselection") storage.HandleJoinSelection(values);
    else if (inlet === 0 && messagename === "action") storage.HandleAction(values[0], values[1]);
    else if (inlet === 1 && messagename === "snapshot") {
        storage.HandleSnapshot(["snapshot"].concat(values));
    }
}
