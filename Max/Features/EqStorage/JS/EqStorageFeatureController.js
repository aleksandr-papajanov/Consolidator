include("../../Shared/JS/DictionaryReader.js");
include("../../Shared/JS/Messages/MessageEnvelope.js");
include("../../Shared/JS/Messages/MessageFactory.js");
include("BankFilter.js");

autowatch = 1;
inlets = 3;
outlets = 7;

// Inlet 0: initialize, bang, add <name>, remove, select <index>, rename <index> <name>, delete <index>.
// Inlet 1: message <dictionary type=filter.define|filter.update|filter.bypass>.
// Inlet 2: dictionary <name> restores state; persistence_ready enables state commits after recall.
// Outlet 0: message <dictionary type=filter.update|filter.bypass|filter.reset> to Filter instances; fit updates may carry bankIndex.
// Outlet 1: clear, append <name>, set <index> for the bank list.
// Outlet 2: error <code>, status <...>.
// Outlet 3: message <dictionary type=filter.define|eq.storage.snapshot> to EqChain.
// Outlet 4: message <dictionary type=filter.define> to Approximator.
// Outlet 5: message <dictionary type=eq.storage.bank.changed>.
// Outlet 6: dictionary <name> with the complete persistent bank state.

function EqStorage() {
    this.state = new Dict();
    this.schemaVersion = 1;
    this.filterOrder = [];
    this.filterDefinitions = {};
    this.selectedRow = 1;
    this.isApplyingBank = false;
    this.recallPendingFilters = {};
    this.isPersistingState = false;
    this.persistenceReady = false;
    this.snapshotSequence = 0;
    this.bankAdjectives = [
        "Neon", "Velvet", "Silent", "Electric", "Golden",
        "Midnight", "Crystal", "Cosmic", "Liquid", "Hidden"
    ];
    this.bankNouns = [
        "Circuit", "Lighthouse", "Orbit", "Machine", "Garden",
        "Signal", "Comet", "Room", "Echo", "Pulse"
    ];
}

EqStorage.prototype.initialize = function() {
    this.initializeStateModel();
    this.publishBankList();
    this.publishBankChanged("selected");
    this.publishEqChainSnapshot();
};

EqStorage.prototype.initializeStateModel = function() {
    if (this.isMissing(this.state.get("schema_version"))) {
        this.state.replace("schema_version", this.schemaVersion);
        this.state.replace("bank_count", 1);
        this.state.replace("selected_row", 1);
    }
    else this.state.replace("schema_version", this.schemaVersion);

    if (this.bankCount() < 1) {
        this.state.replace("bank_count", 1);
        this.state.replace("selected_row", 1);
    }

    this.restoreFilterDefinitions();
    this.ensureBankNames();
    this.selectedRow = this.clampRow(this.state.get("selected_row"));
    this.state.replace("selected_row", this.selectedRow);
};

EqStorage.prototype.dispatch = function(input, command, args) {
    if (input === 2) {
        if (command === "dictionary" && args.length === 1) {
            this.restoreState(args[0]);
            return;
        }
        if (command === "persistence_ready" && args.length === 0) {
            this.enablePersistence();
            return;
        }
        this.emitError("invalid_storage_command");
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

EqStorage.prototype.restoreState = function(dictionaryName) {
    if (this.isPersistingState) {
        return;
    }
    this.state = new Dict(String(dictionaryName));
    this.initialize();
};

EqStorage.prototype.enablePersistence = function() {
    this.persistenceReady = true;
    this.commitState();
};

EqStorage.prototype.handleFilterMessage = function(dictionaryName) {
    var message = MessageFactory.fromMax(dictionaryName);
    if (!message) {
        this.emitError("invalid_message_envelope");
        return;
    }
    if (message.target !== "eq.storage" && message.target !== "broadcast") {
        return;
    }

    if (message.type === "filter.define") {
        var definedFilterId = message.payload.filterId;
        this.rememberFilter(definedFilterId);
        this.forwardFilterDefinition(3, "eq.chain", message);
        this.forwardFilterDefinition(4, "approximator", message);
        this.publishEqChainSnapshot();
        return;
    }

    if (message.type === "filter.update") {
        var updatedFilterId = message.payload.filterId;
        var updateBankIndex = message.payload.bankIndex;
        var hasExplicitBank = updateBankIndex !== undefined &&
            updateBankIndex !== null && updateBankIndex !== "";
        if (this.isApplyingBank && !hasExplicitBank && this.ConsumeRecallFilter(updatedFilterId)) {
            return;
        }
        this.rememberFilter(updatedFilterId);
        this.storeFilterValues(
            updatedFilterId,
            message.payload.values,
            updateBankIndex
        );
        return;
    }

    if (message.type === "filter.bypass") {
        var bypassedFilterId = message.payload.filterId;
        if (this.isApplyingBank && this.ConsumeRecallFilter(bypassedFilterId)) {
            return;
        }
        this.rememberFilter(bypassedFilterId);
        this.storeFilterBypass(bypassedFilterId, message.payload.value);
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
        this.selectBankRow(Number(args[0]));
    } else if (command === "rename" && args.length === 2) {
        this.renameBank(Number(args[0]), String(args[1]));
    } else if (command === "delete" && args.length === 1) {
        this.deleteBank(Number(args[0]));
    } else {
        this.emitError("invalid_ui_command");
    }
};

EqStorage.prototype.addBank = function(label) {
    var index = this.bankCount() + 1;
    var name = label.length > 0 ? label : this.generateBankName(index);
    this.state.replace(this.bankNameKey(index), name);
    this.state.replace("bank_count", index);

    this.selectRow(index, false);
    this.resetAllFilters();
    this.publishEqChainSnapshot();
    this.publishBankChanged("created", null, name, index);
    this.emitStatus("bank_created", index, name);
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
    this.recallPendingFilters = {};
    for (var index = 0; index < this.filterOrder.length; index++) {
        this.recallPendingFilters[String(this.filterOrder[index])] = true;
    }

    if (this.filterOrder.length === 0) {
        this.isApplyingBank = false;
    }

    this.resetAllFilters();

    for (var i = 0; i < this.filterOrder.length; i++) {
        var filter = this.loadStoredFilter(this.filterOrder[i]);
        if (!filter || !filter.isDefined()) {
            continue;
        }
        this.sendFilterMessage(filter.valueMessage());
        this.sendFilterMessage(filter.bypassMessage());
    }

};

EqStorage.prototype.ConsumeRecallFilter = function(id) {
    id = String(id);
    if (!this.recallPendingFilters[id]) {
        return false;
    }

    delete this.recallPendingFilters[id];
    var pendingIds = Object.keys(this.recallPendingFilters);
    if (pendingIds.length === 0) {
        this.isApplyingBank = false;
    }
    return true;
};

EqStorage.prototype.removeSelectedBank = function() {
    if (this.bankCount() <= 1) {
        this.emitError("cannot_remove_last_bank");
        return;
    }
    this.deleteBank(this.selectedRow);
};

EqStorage.prototype.deleteBank = function(row) {
    var index = Number(row);
    var count = this.bankCount();
    if (index < 1 || index > count) {
        this.emitError("invalid_bank_slot");
        return;
    }

    var removedName = this.bankName(index);
    for (var i = index; i < count; i++) {
        this.copyBankRow(i + 1, i);
    }
    this.removeBankRowState(count);
    this.state.replace("bank_count", count - 1);
    this.selectRow(Math.min(index, count - 1));
    this.publishEqChainSnapshot();
    this.publishBankChanged("removed", null, removedName, index);
};

EqStorage.prototype.renameBank = function(row, name) {
    var index = Number(row);
    if (index < 1 || index > this.bankCount() || name.length < 1) {
        this.emitError("invalid_bank_name");
        return;
    }
    this.state.replace(this.bankNameKey(index), name);
    this.publishBankList();
    this.publishEqChainSnapshot();
    this.publishBankChanged("renamed", null, name, index);
};

EqStorage.prototype.storeFilterValues = function(id, values, bankIndex) {
    if (this.isApplyingBank && (bankIndex === undefined || bankIndex === null || bankIndex === "")) {
        return;
    }
    if (bankIndex !== undefined && bankIndex !== null && bankIndex !== "") {
        this.storeFilterValuesAtBank(id, values, Number(bankIndex));
        return;
    }

    var filter = this.loadStoredFilter(id) || new BankFilter(id, [], 0);
    filter.values = normalizeFilterValues(values);
    this.saveStoredFilter(filter);
    this.publishEqChainSnapshot();
    this.publishBankChanged("updated", id, null, null, filter);
};

EqStorage.prototype.storeFilterValuesAtBank = function(id, values, bankIndex) {
    bankIndex = Math.floor(Number(bankIndex));
    if (!isFinite(bankIndex) || bankIndex < 1 || bankIndex > this.bankCount()) {
        this.emitError("invalid_bank_slot");
        return;
    }

    var filter = this.loadStoredFilterAtBank(id, bankIndex) || new BankFilter(id, [], 0);
    filter.values = normalizeFilterValues(values);
    this.saveStoredFilterAtBank(filter, bankIndex);
    this.publishEqChainSnapshot();
    this.publishBankChanged("updated", id, null, bankIndex, filter);
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

EqStorage.prototype.loadStoredFilterAtBank = function(id, bankIndex) {
    var values = this.state.get(this.filterPath(bankIndex, id));
    if (this.isMissing(values)) {
        return null;
    }
    var bypass = this.state.get(this.bypassPath(bankIndex, id));
    return new BankFilter(id, values, this.numberOrDefault(bypass, 0));
};

EqStorage.prototype.saveStoredFilterAtBank = function(filter, bankIndex) {
    this.state.replace(this.filterPath(bankIndex, filter.id), filter.values);
    this.state.replace(this.bypassPath(bankIndex, filter.id), filter.bypass);
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
            "filter.reset", "filter", {
                filterId: Number(this.filterOrder[i])
            }, "eq.storage"
        ));
    }
};

EqStorage.prototype.sendFilterMessage = function(message) {
    this.sendEnvelope(0, message);
};

EqStorage.prototype.forwardFilterDefinition = function(outletIndex, target, message) {
    var payload = {
        filterId: Number(message.payload.filterId),
        contractName: String(message.payload.contractName)
    };
    this.sendEnvelope(outletIndex, MessageEnvelope.create(
        "filter.define", target, payload, "eq.storage"
    ));
};

EqStorage.prototype.publishEqChainSnapshot = function() {
    var name = "consolidator.eqstorage.snapshot." + (++this.snapshotSequence);
    var snapshot = new Dict(name);
    snapshot.clear();
    snapshot.setparse("banks", "{}");

    for (var row = 1; row <= this.bankCount(); row++) {
        var bankPath = "banks::" + row;
        snapshot.replace(bankPath + "::name", this.bankName(row));
        snapshot.setparse(bankPath + "::filters", "{}");
        for (var index = 0; index < this.filterOrder.length; index++) {
            var filterId = this.filterOrder[index];
            var values = this.state.get(this.filterPath(row, filterId));
            if (this.isMissing(values)) {
                continue;
            }
            var filterPath = bankPath + "::filters::" + filterId;
            snapshot.replace(filterPath + "::values", normalizeFilterValues(values));
            snapshot.replace(
                filterPath + "::bypass",
                this.numberOrDefault(this.state.get(this.bypassPath(row, filterId)), 0)
            );
        }
    }

    this.sendEnvelope(3, MessageEnvelope.create(
        "eq.storage.snapshot", "eq.chain", { snapshotName: name }, "eq.storage"
    ));
    this.commitState();
};

EqStorage.prototype.commitState = function() {
    if (!this.persistenceReady) {
        return;
    }
    this.isPersistingState = true;
    outlet(6, "dictionary", this.state.name);
    this.isPersistingState = false;
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
            ? this.bankName(bankRow)
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
        "eq.storage.bank.changed", "broadcast", payload, "eq.storage"
    ));
};

EqStorage.prototype.publishBankList = function() {
    outlet(1, "clear");
    for (var displayRow = 0; displayRow < this.bankCount(); displayRow++) {
        var storageRow = this.bankCount() - displayRow;
        outlet(1, "append", storageRow + " " + this.bankName(storageRow), storageRow);
    }
    outlet(1, "setid", this.selectedRow);
};

EqStorage.prototype.selectBankRow = function(row) {
    row = Math.floor(Number(row));
    var count = this.bankCount();
    if (!isFinite(row) || row < 1 || row > count) {
        this.emitError("invalid_bank_slot");
        return;
    }
    this.selectRow(row);
};

EqStorage.prototype.ensureBankNames = function() {
    for (var i = 1; i <= this.bankCount(); i++) {
        var key = this.bankNameKey(i);
        if (this.isMissing(this.state.get(key))) {
            this.state.replace(key, this.generateBankName(i));
        }
    }
};

EqStorage.prototype.generateBankName = function(index) {
    var attempt = 0;
    while (attempt < 100) {
        var adjective = this.bankAdjectives[(index + attempt) % this.bankAdjectives.length];
        var noun = this.bankNouns[Math.floor((index + attempt) / this.bankAdjectives.length) % this.bankNouns.length];
        var candidate = adjective + " " + noun;
        var duplicate = false;
        for (var i = 1; i <= this.bankCount(); i++) {
            var existingName = this.state.get(this.bankNameKey(i));
            if (!this.isMissing(existingName) && String(existingName) === candidate) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            return candidate;
        }
        attempt++;
    }
    return "Bank " + (index + 1);
};

EqStorage.prototype.copyBankRow = function(from, to) {
    this.state.replace(this.bankNameKey(to), this.state.get(this.bankNameKey(from)));
    for (var i = 0; i < this.filterOrder.length; i++) {
        var id = this.filterOrder[i];
        var filter = this.loadStoredFilterAtBank(id, from);
        if (filter) {
            this.saveStoredFilterAtBank(filter, to);
        } else {
            this.removeStoredFilterAtBank(id, to);
        }
    }
};

EqStorage.prototype.removeBankRowState = function(row) {
    this.removeBankState(row);
};

EqStorage.prototype.removeBankState = function(index) {
    this.state.remove(this.bankNameKey(index));
    for (var i = 0; i < this.filterOrder.length; i++) {
        this.state.remove(this.filterPath(index, this.filterOrder[i]));
        this.state.remove(this.bypassPath(index, this.filterOrder[i]));
    }
};

EqStorage.prototype.removeStoredFilterAtBank = function(id, bankIndex) {
    this.state.remove(this.filterPath(bankIndex, id));
    this.state.remove(this.bypassPath(bankIndex, id));
};

EqStorage.prototype.activeFilterPath = function(id) {
    return this.filterPath(this.selectedRow, id);
};

EqStorage.prototype.activeBypassPath = function(id) {
    return this.bypassPath(this.selectedRow, id);
};

EqStorage.prototype.filterPath = function(index, id) { return "bank_" + index + "_filter_" + id; };
EqStorage.prototype.bypassPath = function(index, id) { return "bank_" + index + "_bypass_" + id; };
EqStorage.prototype.bankNameKey = function(index) { return "bank_" + index + "_name"; };
EqStorage.prototype.bankCount = function() { return this.numberOrDefault(this.state.get("bank_count"), 0); };
EqStorage.prototype.selectedBankName = function() { return this.bankName(this.selectedRow); };
EqStorage.prototype.bankName = function(index) {
    var value = this.state.get(this.bankNameKey(index));
    return this.isMissing(value) ? this.generateBankName(index) : String(value);
};
EqStorage.prototype.clampRow = function(row) {
    return Math.max(1, Math.min(
        Math.floor(this.numberOrDefault(row, 0)),
        this.bankCount()
    ));
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

function loadbang() {
    eqStorage.initializeStateModel();
    eqStorage.publishBankList();
    eqStorage.publishBankChanged("selected");
}

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
function msg_int(value) { if (inlet === 0) eqStorage.selectBankRow(value); }
function message() { eqStorage.dispatch(inlet, "message", arrayfromargs(arguments)); }
function list() {
    var args = arrayfromargs(arguments);
    if (args.length > 0) eqStorage.dispatch(inlet, String(args[0]), args.slice(1));
}
function anything() { eqStorage.dispatch(inlet, messagename, arrayfromargs(arguments)); }
