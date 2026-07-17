include("../Messages/MessageEnvelope.js");
include("../Messages/MessageFactory.js");
include("BankFilter.js");

autowatch = 1;
inlets = 3;
outlets = 7;

var storageDictionaryName = null;

// Inlet 0: initialize, bang, add <name>, remove, select <index>, rename <index> <name>, delete <index>.
// Inlet 1: message <dictionary type=filter.define|filter.update|filter.bypass>.
// Inlet 2: dictionary <embedded-dictionary-name>.
// Outlet 0: message <dictionary type=filter.update|filter.bypass|filter.reset> to Filter instances.
// Outlet 1: clear, append <name>, set <index> for the bank list.
// Outlet 2: error <code>, status <...>.
// Outlet 3: message <dictionary type=filter.define|eq.storage.snapshot> to EqChain.
// Outlet 4: message <dictionary type=filter.define> to Approximator.
// Outlet 5: message <dictionary type=eq.storage.bank.changed>.
// Outlet 6: dictionary <name> commits the current state to the parent dict.

function EqStorage() {
    this.state = null;
    this.schemaVersion = 5;
    this.filterOrder = [];
    this.filterDefinitions = {};
    this.selectedRow = 0;
    this.isApplyingBank = false;
    this.snapshotSequence = 0;
    this.pendingFilterMessages = [];
}

EqStorage.prototype.initialize = function() {
    if (this.isMissing(this.state.get("schema_version"))) {
        this.state.replace("schema_version", this.schemaVersion);
        this.state.replace("bank_count", 0);
        this.state.replace("selected_row", 0);
    }
    else {
        this.state.replace("schema_version", this.schemaVersion);
    }

    this.restoreFilterDefinitions();
    this.state.replace(this.currentBankNameKey(), "Current");
    this.selectedRow = this.clampRow(this.state.get("selected_row"));
    this.state.replace("selected_row", this.selectedRow);
    this.publishBankList();
    this.publishEqChainSnapshot();
};

EqStorage.prototype.dispatch = function(input, command, args) {
    if (input === 2) {
        if (command !== "dictionary" || args.length !== 1) {
            this.emitError("invalid_storage_dictionary");
            return;
        }
        this.bindStorageDictionary(args[0]);
        return;
    }
    if (input === 0) {
        this.handleUiCommand(command, args);
        return;
    }
    if (command === "message") {
        this.handleFilterMessage(args);
        return;
    }
    this.emitError("unsupported_message_selector");
};

EqStorage.prototype.bindStorageDictionary = function(name) {
    if (this.state) {
        return;
    }

    storageDictionaryName = String(name);
    this.state = new Dict(storageDictionaryName);
    this.initialize();

    var messages = this.pendingFilterMessages;
    this.pendingFilterMessages = [];
    for (var i = 0; i < messages.length; i++) {
        this.handleFilterMessage(messages[i]);
    }
};

EqStorage.prototype.handleFilterMessage = function(dictionaryName) {
    if (!this.state) {
        this.pendingFilterMessages.push(dictionaryName);
        return;
    }

    var message = MessageFactory.fromMax(dictionaryName);
    if (!message) {
        this.emitError("invalid_message_envelope");
        return;
    }

    if (message.type === "filter.define") {
        this.rememberFilter(message.target);
        this.forwardIncomingEnvelope(3, dictionaryName);
        this.forwardIncomingEnvelope(4, dictionaryName);
        this.publishEqChainSnapshot();
        return;
    }

    if (message.type === "filter.update") {
        this.rememberFilter(message.target);
        this.storeFilterValues(message.target, message.payloadValue("values"));
        return;
    }

    if (message.type === "filter.bypass") {
        this.rememberFilter(message.target);
        this.storeFilterBypass(message.target, message.payloadValue("value"));
        return;
    }

    this.emitError("unsupported_filter_message");
};

EqStorage.prototype.handleUiCommand = function(command, args) {
    if (command === "initialize" || command === "bang") {
        this.initialize();
    } else if (command === "add") {
        this.addBank(args.length > 0 ? String(args[0]) : "");
    } else if (command === "remove") {
        this.removeSelectedBank();
    } else if (command === "select" && args.length === 1) {
        this.selectRow(Number(args[0]));
    } else if (command === "rename" && args.length === 2) {
        this.renameBank(Number(args[0]), String(args[1]));
    } else if (command === "delete" && args.length === 1) {
        this.deleteBank(Number(args[0]));
    } else {
        this.emitError("invalid_ui_command");
    }
};

EqStorage.prototype.addBank = function(label) {
    var index = this.bankCount();
    var name = label.length > 0 ? label : "EQ " + (index + 1);
    this.state.replace(this.bankNameKey(index), name);
    this.state.replace("bank_count", index + 1);
    this.selectRow(index + 1, false);

    // A new bank is filled by the Filter reset events. No state is queried.
    this.resetAllFilters();
    this.publishEqChainSnapshot();
    this.publishBankChanged("created");
    this.emitStatus("bank_created", index + 1, name);
};

EqStorage.prototype.selectRow = function(row, applyValues) {
    row = this.clampRow(row);
    this.selectedRow = row;
    this.state.replace("selected_row", row);
    this.publishBankList();

    if (applyValues !== false) {
        this.applySelectedBank();
    }

    this.publishBankChanged("selected");
    this.publishEqChainSnapshot();
    this.emitStatus("selected", row, this.selectedBankName());
};

EqStorage.prototype.applySelectedBank = function() {
    this.isApplyingBank = true;
    this.resetAllFilters();

    for (var i = 0; i < this.filterOrder.length; i++) {
        var filter = this.loadStoredFilter(this.filterOrder[i]);
        if (!filter || !filter.isDefined()) {
            continue;
        }
        this.sendFilterMessage(filter.valueMessage());
        this.sendFilterMessage(filter.bypassMessage());
    }

    this.isApplyingBank = false;
};

EqStorage.prototype.removeSelectedBank = function() {
    if (this.selectedRow < 1) {
        this.emitError("cannot_remove_current");
        return;
    }
    this.deleteBank(this.selectedRow);
};

EqStorage.prototype.deleteBank = function(row) {
    var index = Number(row) - 1;
    var count = this.bankCount();
    if (index < 0 || index >= count) {
        this.emitError("invalid_bank_slot");
        return;
    }

    var removedName = this.bankName(index);
    for (var i = index; i < count - 1; i++) {
        this.copyBank(i + 1, i);
    }
    this.removeBankState(count - 1);
    this.state.replace("bank_count", count - 1);
    this.selectRow(0);
    this.publishEqChainSnapshot();
    this.publishBankChanged("removed", null, removedName, index + 1);
};

EqStorage.prototype.renameBank = function(row, name) {
    var index = Number(row) - 1;
    if (index < 0 || index >= this.bankCount() || name.length < 1) {
        this.emitError("invalid_bank_name");
        return;
    }
    this.state.replace(this.bankNameKey(index), name);
    this.publishBankList();
    this.publishEqChainSnapshot();
    this.publishBankChanged("renamed", null, name, index + 1);
};

EqStorage.prototype.storeFilterValues = function(id, values) {
    if (this.isApplyingBank) {
        return;
    }
    var filter = this.loadStoredFilter(id) || new BankFilter(id, [], 0);
    filter.values = normalizeFilterValues(values);
    this.saveStoredFilter(filter);
    this.publishEqChainSnapshot();
    this.publishBankChanged("updated", id, null, null, filter);
};

EqStorage.prototype.storeFilterBypass = function(id, bypass) {
    if (this.isApplyingBank) {
        return;
    }
    var filter = this.loadStoredFilter(id) || new BankFilter(id, [], 0);
    filter.bypass = this.numberOrDefault(bypass, 0) === 1 ? 1 : 0;
    this.saveStoredFilter(filter);
    this.publishEqChainSnapshot();
    this.publishBankChanged("updated", id, null, null, filter);
};

EqStorage.prototype.loadStoredFilter = function(id) {
    var values = this.state.get(this.activeFilterPath(id));
    if (this.isMissing(values)) {
        return null;
    }
    return new BankFilter(
        id,
        values,
        this.numberOrDefault(this.state.get(this.activeBypassPath(id)), 0)
    );
};

EqStorage.prototype.saveStoredFilter = function(filter) {
    this.state.replace(this.activeFilterPath(filter.id), filter.values);
    this.state.replace(this.activeBypassPath(filter.id), filter.bypass);
};

EqStorage.prototype.rememberFilter = function(id) {
    id = String(id);
    if (this.filterDefinitions[id]) {
        return;
    }
    this.filterDefinitions[id] = true;
    this.filterOrder.push(id);
    this.state.replace("filter_ids", this.filterOrder);
};

EqStorage.prototype.restoreFilterDefinitions = function() {
    var ids = normalizeFilterValues(this.state.get("filter_ids"));
    this.filterOrder = [];
    this.filterDefinitions = {};
    for (var i = 0; i < ids.length; i++) {
        this.rememberFilter(ids[i]);
    }
};

EqStorage.prototype.resetAllFilters = function() {
    for (var i = 0; i < this.filterOrder.length; i++) {
        this.sendFilterMessage(MessageEnvelope.create(
            "filter.reset", Number(this.filterOrder[i]), {}, "eq.storage"
        ));
    }
};

EqStorage.prototype.sendFilterMessage = function(message) {
    this.sendEnvelope(0, message);
};

EqStorage.prototype.forwardIncomingEnvelope = function(outletIndex, dictionaryName) {
    var name = MessageEnvelope.dictionaryName(dictionaryName);
    if (!name) {
        this.emitError("invalid_message_envelope");
        return;
    }
    outlet(outletIndex, "message", name);
};

EqStorage.prototype.publishEqChainSnapshot = function() {
    var name = "consolidator.eqstorage.snapshot." + (++this.snapshotSequence);
    var snapshot = new Dict(name);
    snapshot.clear();
    snapshot.setparse("banks", "{}");

    for (var row = 0; row <= this.bankCount(); row++) {
        var bankPath = "banks::" + row;
        snapshot.replace(bankPath + "::name", row === 0 ? "Current" : this.bankName(row - 1));
        snapshot.setparse(bankPath + "::filters", "{}");
        for (var index = 0; index < this.filterOrder.length; index++) {
            var filterId = this.filterOrder[index];
            var values = row === 0
                ? this.state.get(this.currentFilterPath(filterId))
                : this.state.get(this.filterPath(row - 1, filterId));
            if (this.isMissing(values)) {
                continue;
            }
            var filterPath = bankPath + "::filters::" + filterId;
            snapshot.replace(filterPath + "::values", normalizeFilterValues(values));
            snapshot.replace(
                filterPath + "::bypass",
                row === 0
                    ? this.numberOrDefault(this.state.get(this.currentBypassPath(filterId)), 0)
                    : this.numberOrDefault(this.state.get(this.bypassPath(row - 1, filterId)), 0)
            );
        }
    }

    this.sendEnvelope(3, MessageEnvelope.create(
        "eq.storage.snapshot", null, { snapshotName: name }, "eq.storage"
    ));
    this.commitState();
};

EqStorage.prototype.commitState = function() {
    outlet(6, "dictionary", storageDictionaryName);
};

EqStorage.prototype.sendEnvelope = function(outletIndex, message) {
    var dictionary = MessageFactory.toMax(message);
    if (!dictionary) {
        this.emitError("invalid_message_envelope");
        return;
    }
    outlet(outletIndex, "message", dictionary.name);
};

EqStorage.prototype.publishBankChanged = function(action, filterId, name, row, filter) {
    var bankRow = row === undefined || row === null ? this.selectedRow : Number(row);
    var payload = {
        action: String(action),
        bankIndex: bankRow,
        bankName: name === undefined || name === null
            ? (bankRow === 0 ? "Current" : this.bankName(bankRow - 1))
            : String(name)
    };
    if (filterId !== undefined && filterId !== null) {
        payload.filterId = Number(filterId);
    }
    if (filter) {
        payload.values = filter.values;
        payload.bypass = filter.bypass;
    }
    this.sendEnvelope(5, MessageEnvelope.create(
        "eq.storage.bank.changed", null, payload, "eq.storage"
    ));
};

EqStorage.prototype.publishBankList = function() {
    outlet(1, "clear");
    outlet(1, "append", "Current");
    for (var i = 0; i < this.bankCount(); i++) {
        outlet(1, "append", this.bankName(i));
    }
    outlet(1, "set", this.selectedRow);
};

EqStorage.prototype.copyBank = function(from, to) {
    this.state.replace(this.bankNameKey(to), this.state.get(this.bankNameKey(from)));
    for (var i = 0; i < this.filterOrder.length; i++) {
        var id = this.filterOrder[i];
        this.state.replace(this.filterPath(to, id), this.state.get(this.filterPath(from, id)));
        this.state.replace(this.bypassPath(to, id), this.state.get(this.bypassPath(from, id)));
    }
};

EqStorage.prototype.removeBankState = function(index) {
    this.state.remove(this.bankNameKey(index));
    for (var i = 0; i < this.filterOrder.length; i++) {
        this.state.remove(this.filterPath(index, this.filterOrder[i]));
        this.state.remove(this.bypassPath(index, this.filterOrder[i]));
    }
};

EqStorage.prototype.activeFilterPath = function(id) {
    return this.selectedRow === 0
        ? this.currentFilterPath(id)
        : this.filterPath(this.selectedRow - 1, id);
};

EqStorage.prototype.activeBypassPath = function(id) {
    return this.selectedRow === 0
        ? this.currentBypassPath(id)
        : this.bypassPath(this.selectedRow - 1, id);
};

EqStorage.prototype.currentFilterPath = function(id) { return "bank_current_filter_" + id; };
EqStorage.prototype.currentBypassPath = function(id) { return "bank_current_bypass_" + id; };
EqStorage.prototype.currentBankNameKey = function() { return "bank_current_name"; };
EqStorage.prototype.filterPath = function(index, id) { return "bank_" + index + "_filter_" + id; };
EqStorage.prototype.bypassPath = function(index, id) { return "bank_" + index + "_bypass_" + id; };
EqStorage.prototype.bankNameKey = function(index) { return "bank_" + index + "_name"; };
EqStorage.prototype.bankCount = function() { return this.numberOrDefault(this.state.get("bank_count"), 0); };
EqStorage.prototype.selectedBankName = function() { return this.selectedRow === 0 ? "Current" : this.bankName(this.selectedRow - 1); };
EqStorage.prototype.bankName = function(index) {
    var value = this.state.get(this.bankNameKey(index));
    return this.isMissing(value) ? "EQ " + (index + 1) : String(value);
};
EqStorage.prototype.clampRow = function(row) {
    return Math.max(0, Math.min(Math.floor(this.numberOrDefault(row, 0)), this.bankCount()));
};
EqStorage.prototype.numberOrDefault = function(value, fallback) {
    var number = Number(value);
    return isFinite(number) ? number : fallback;
};
EqStorage.prototype.isMissing = function(value) { return value === null || value === undefined || value === ""; };
EqStorage.prototype.emitStatus = function() {
    var args = arrayfromargs(arguments);
    if (args.length === 1) outlet(2, "status", args[0]);
    else if (args.length === 2) outlet(2, "status", args[0], args[1]);
    else outlet(2, "status", args[0], args[1], args[2]);
};
EqStorage.prototype.emitError = function(code) { outlet(2, "error", code); };

var eqStorage = new EqStorage();

function DispatchUiCommand(command, args) {
    if (inlet === 0) eqStorage.handleUiCommand(command, args);
}

function initialize() { DispatchUiCommand("initialize", []); }
function bang() { DispatchUiCommand("bang", []); }
function add() { DispatchUiCommand("add", arrayfromargs(arguments)); }
function remove() { DispatchUiCommand("remove", []); }
function select() { DispatchUiCommand("select", arrayfromargs(arguments)); }
function rename() { DispatchUiCommand("rename", arrayfromargs(arguments)); }
function delete_bank() { DispatchUiCommand("delete", arrayfromargs(arguments)); }
function msg_int(value) { if (inlet === 0) eqStorage.selectRow(value); }
function message() { eqStorage.dispatch(inlet, "message", arrayfromargs(arguments)); }
function dictionary() { eqStorage.dispatch(inlet, "dictionary", arrayfromargs(arguments)); }
function list() {
    var args = arrayfromargs(arguments);
    if (args.length > 0) eqStorage.dispatch(inlet, String(args[0]), args.slice(1));
}
function anything() { eqStorage.dispatch(inlet, messagename, arrayfromargs(arguments)); }
