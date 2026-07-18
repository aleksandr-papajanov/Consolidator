include("../Shared/JS/DictionaryReader.js");
include("../Shared/JS/Messages/MessageEnvelope.js");
include("../Shared/JS/Messages/MessageFactory.js");
include("JS/BankFilter.js");
include("JS/DeviceStateStore.js");

autowatch = 1;
inlets = 3;
outlets = 7;

// Inlet 0: initialize, bang, add <name>, remove, select <index>, rename <index> <name>, delete <index>.
// Inlet 1: message <dictionary type=filter.control|filter.set|filter.set_many|filter.reset|system.status>.
// Inlet 2: dictionary <name> restores state; persistence_ready enables state commits after recall.
// Outlet 0: reserved.
// Outlet 1: clear, append <name>, set <index> for the bank list.
// Outlet 2: error <code>, status <...>.
// Outlet 3: message <dictionary type=device.state.changed> to EqChain.
// Outlet 4: reserved.
// Outlet 5: message <dictionary type=filter.state|device.state.changed|system.status> to BusHub.
// Outlet 6: dictionary <name> with the complete persistent bank state.

function EqStorage() {
    this.store = new DeviceStateStore(this.HandleStoreCommit, this);
    this.schemaVersion = 3;
    this.initialized = false;
    this.filterOrder = [];
    this.filterDefinitions = {};
    this.selectedRow = 1;
    this.isPersistingState = false;
    this.persistenceReady = false;
    this.started = false;
    this.persistenceTask = new Task(this.commitState, this);
    this.bankAdjectives = [
        "Neon", "Velvet", "Silent", "Electric", "Golden",
        "Midnight", "Crystal", "Cosmic", "Liquid", "Hidden"
    ];
    this.bankNouns = [
        "Circuit", "Lighthouse", "Orbit", "Machine", "Garden",
        "Signal", "Comet", "Room", "Echo", "Pulse"
    ];
}

EqStorage.ConfigurationPath = "Config/FilterConfig.json";

EqStorage.prototype.initialize = function() {
    this.initializeStateModel();
    this.publishBankList();
    this.PublishStateNow();
};

EqStorage.prototype.initializeStateModel = function() {
    if (!this.LoadFilterDefinitions()) return;

    if (this.numberOrDefault(this.store.Get("schema_version"), 0) !== this.schemaVersion) {
        this.store.Clear();
        this.store.Replace("schema_version", this.schemaVersion);
        this.store.Replace("revision", 0);
        this.store.Replace("generation", 0);
        this.store.Replace("bank_count", 1);
        this.store.Replace("selected_row", 1);
        this.store.SetParse("filters", "{}");
    }

    if (this.bankCount() < 1) {
        this.store.Replace("bank_count", 1);
        this.store.Replace("selected_row", 1);
    }

    this.ensureBankNames();
    this.WriteDefinitionsToStore();
    this.selectedRow = this.clampRow(this.store.Get("selected_row"));
    this.store.Replace("selected_row", this.selectedRow);
    this.initialized = true;
    this.EnsureAllDefinedFiltersInBanks();
};

EqStorage.prototype.EnsureInitialized = function() {
    if (!this.initialized) {
        this.initializeStateModel();
    }
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
    this.store.Attach(dictionaryName);
    this.initialized = false;
    this.initialize();
};

EqStorage.prototype.enablePersistence = function() {
    this.persistenceReady = true;
    this.initializeStateModel();
    this.publishBankList();
    this.PublishStateNow();
    this.sendEnvelope(5, MessageEnvelope.create(
        "system.status", "bus.hub",
        { feature: "eq.storage", state: "ready" }, "eq.storage"));
};

EqStorage.prototype.handleFilterMessage = function(dictionaryName) {
    this.EnsureInitialized();
    var message = MessageFactory.fromMax(dictionaryName);
    if (!message) {
        this.emitError("invalid_message_envelope");
        return;
    }
    if (message.type === "system.start" && message.target === "broadcast") {
        if (!this.started) {
            this.started = true;
            this.applySelectedBank();
            this.PublishStateNow();
        }
        return;
    }
    if (message.type === "system.status" &&
            message.payload &&
            message.payload.feature === "filter" &&
            message.payload.state === "ready") {
        this.PublishFilterState(message.payload.filterId, this.selectedRow);
        return;
    }
    if (message.target !== "eq.storage") {
        return;
    }

    if (message.type === "filter.control") return this.ApplyFilterControl(message.payload);
    if (message.type === "filter.set") return this.ApplyFilterSet(message.payload);
    if (message.type === "filter.set_many") return this.ApplyFilterSetMany(message.payload);
    if (message.type === "filter.reset") return this.ResetFilter(message.payload);

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
    this.store.Replace(this.bankNameKey(index), name);
    this.store.Replace("bank_count", index);
    this.initializeBankDefaults(index);
    this.selectRow(index, false);
    this.applySelectedBank();
    this.emitStatus("bank_created", index, name);
};

EqStorage.prototype.selectRow = function(row, applyValues) {
    row = this.clampRow(row);
    this.selectedRow = row;
    this.store.Replace("selected_row", row);
    this.publishBankList();

    if (applyValues !== false) {
        this.applySelectedBank();
    }

    this.PublishStateNow();
    this.emitStatus("selected", row, this.selectedBankName());
};

EqStorage.prototype.applySelectedBank = function() {
    for (var i = 0; i < this.filterOrder.length; i++) {
        this.PublishFilterState(this.filterOrder[i], this.selectedRow);
    }
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
    this.store.Replace("bank_count", count - 1);
    this.selectRow(Math.min(index, count - 1));
};

EqStorage.prototype.renameBank = function(row, name) {
    var index = Number(row);
    if (index < 1 || index > this.bankCount() || name.length < 1) {
        this.emitError("invalid_bank_name");
        return;
    }
    this.store.Replace(this.bankNameKey(index), name);
    this.publishBankList();
    this.PublishStateNow();
};

EqStorage.prototype.ApplyFilterControl = function(payload) {
    var id = String(payload.filterId);
    var definition = this.filterDefinitions[id];
    var filter = this.loadStoredFilter(id);
    var control = String(payload.control || "");
    var normalized = Number(payload.value);
    if (!definition || !definition.source || !filter || !isFinite(normalized) || normalized < 0 || normalized > 1) {
        this.emitError("invalid_filter_control");
        return;
    }
    if (control === "bypass") {
        filter.bypass = normalized >= 0.5 ? 1 : 0;
    } else {
        var parameterIndex = this.FindControlParameterIndex(definition.source, control);
        if (parameterIndex < 0) {
            this.emitError("invalid_filter_control");
            return;
        }
        var parameter = this.ParameterEntries(definition.source)[parameterIndex];
        filter.values[parameterIndex] = this.Denormalize(parameter, normalized);
    }
    this.saveStoredFilter(filter);
    this.PublishFilterState(id, this.selectedRow);
    this.RequestStatePublish();
};

EqStorage.prototype.ApplyFilterSet = function(payload) {
    var id = String(payload.filterId);
    var definition = this.filterDefinitions[id];
    var filter = this.loadStoredFilter(id);
    if (!definition || !definition.source || !filter) {
        this.emitError("invalid_filter_set");
        return;
    }
    var parameters = this.ParameterEntries(definition.source);
    if (payload.parameter === "q") {
        var qIndex = this.FindParameterIndex(parameters, "q");
        if (qIndex < 0) return this.emitError("invalid_filter_set");
        filter.values[qIndex] = this.ClampParameter(parameters[qIndex], Number(payload.value));
    } else {
        var gainIndex = this.FindParameterIndex(parameters, "gain");
        var frequencyIndex = this.FindFrequencyParameterIndex(parameters);
        if (gainIndex >= 0 && payload.gain !== undefined) {
            filter.values[gainIndex] = this.ClampParameter(parameters[gainIndex], Number(payload.gain));
        }
        if (frequencyIndex >= 0 && payload.frequency !== undefined) {
            filter.values[frequencyIndex] = this.ClampParameter(parameters[frequencyIndex], Number(payload.frequency));
        }
    }
    this.saveStoredFilter(filter);
    this.PublishFilterState(id, this.selectedRow);
    this.RequestStatePublish();
};

EqStorage.prototype.ApplyFilterSetMany = function(payload) {
    var id = String(payload.filterId);
    var bankIndex = payload.bankIndex === undefined ? this.selectedRow : Math.floor(Number(payload.bankIndex));
    var definition = this.filterDefinitions[id];
    var parameters = definition && definition.source ? this.ParameterEntries(definition.source) : [];
    var values = payload.values instanceof Array
        ? payload.values
        : (payload.values === undefined ? [] : [payload.values]);
    if (bankIndex < 1 || bankIndex > this.bankCount() || parameters.length !== values.length) {
        this.emitError("invalid_filter_set_many");
        return;
    }
    var filter = this.loadStoredFilterAtBank(id, bankIndex) || new BankFilter(id, [], 0);
    filter.values = [];
    for (var index = 0; index < parameters.length; index++) {
        filter.values.push(this.ClampParameter(parameters[index], Number(values[index])));
    }
    filter.bypass = Number(payload.bypass) === 1 ? 1 : 0;
    this.saveStoredFilterAtBank(filter, bankIndex);
    if (bankIndex === this.selectedRow) this.PublishFilterState(id, bankIndex);
    this.RequestStatePublish();
};

EqStorage.prototype.ResetFilter = function(payload) {
    var id = String(payload.filterId);
    var definition = this.filterDefinitions[id];
    if (!definition || !definition.source) return this.emitError("invalid_filter_reset");
    var filter = new BankFilter(id, definition.defaultValues, definition.defaultBypass);
    this.saveStoredFilter(filter);
    this.PublishFilterState(id, this.selectedRow);
    this.RequestStatePublish();
};

EqStorage.prototype.PublishFilterState = function(id, bankIndex) {
    var definition = this.filterDefinitions[String(id)];
    var filter = this.loadStoredFilterAtBank(id, bankIndex);
    if (!definition || !definition.source || !filter) return;
    var parameters = this.ParameterEntries(definition.source);
    var normalizedValues = [];
    for (var index = 0; index < parameters.length; index++) {
        normalizedValues.push(this.Normalize(parameters[index], filter.values[index]));
    }
    var payload = {
        filterId: Number(id),
        bankIndex: Number(bankIndex),
        values: filter.values,
        normalizedValues: normalizedValues,
        bypass: filter.bypass,
        gain: this.ParameterValue(parameters, filter.values, "gain", 0),
        frequency: this.FrequencyValue(parameters, filter.values),
        q: this.ParameterValue(parameters, filter.values, "q", 0)
    };
    this.sendEnvelope(5, MessageEnvelope.create("filter.state", "filter", payload, "eq.storage"));
    this.sendEnvelope(5, MessageEnvelope.create("filter.state", "spectrum", payload, "eq.storage"));
};

EqStorage.prototype.loadStoredFilter = function(id) {
    var values = this.store.Get(this.activeFilterPath(id));
    if (this.isMissing(values)) {
        return null;
    }
    return new BankFilter(
        id,
        values,
        this.numberOrDefault(this.store.Get(this.activeBypassPath(id)), 0)
    );
};

EqStorage.prototype.saveStoredFilter = function(filter) {
    this.store.Replace(this.activeFilterPath(filter.id), filter.values);
    this.store.Replace(this.activeBypassPath(filter.id), filter.bypass);
};

EqStorage.prototype.loadStoredFilterAtBank = function(id, bankIndex) {
    var values = this.store.Get(this.filterPath(bankIndex, id));
    if (this.isMissing(values)) {
        return null;
    }
    var bypass = this.store.Get(this.bypassPath(bankIndex, id));
    return new BankFilter(id, values, this.numberOrDefault(bypass, 0));
};

EqStorage.prototype.saveStoredFilterAtBank = function(filter, bankIndex) {
    this.store.Replace(this.filterPath(bankIndex, filter.id), filter.values);
    this.store.Replace(this.bypassPath(bankIndex, filter.id), filter.bypass);
};

EqStorage.prototype.LoadFilterDefinitions = function() {
    try {
        var configurationDictionary = new Dict();
        configurationDictionary.import_json(EqStorage.ConfigurationPath);
        var configuration = new DictionaryReader(configurationDictionary.name);
        var filters = configuration.filters || {};
        var ids = [];
        for (var key in filters) {
            if (Object.prototype.hasOwnProperty.call(filters, key)) ids.push(String(key));
        }
        ids.sort(function(left, right) { return Number(left) - Number(right); });
        if (ids.length === 0) throw new Error("missing_filter_definitions");

        this.filterOrder = ids;
        this.filterDefinitions = {};
        for (var index = 0; index < ids.length; index++) {
            var id = ids[index];
            var source = filters[id];
            var parameters = this.ParameterEntries(source);
            var defaults = [];
            for (var parameterIndex = 0; parameterIndex < parameters.length; parameterIndex++) {
                defaults.push(Number(source.parameters[parameters[parameterIndex].name]["default"]));
            }
            this.filterDefinitions[id] = {
                defaultValues: defaults,
                defaultBypass: 0,
                source: source
            };
        }
        return true;
    }
    catch (error) {
        this.filterOrder = [];
        this.filterDefinitions = {};
        this.emitError("invalid_filter_configuration_dictionary");
        return false;
    }
};

EqStorage.prototype.WriteDefinitionsToStore = function() {
    this.store.Replace("filter_order", this.filterOrder.map(Number));
    this.store.SetParse("filters", "{}");
    for (var index = 0; index < this.filterOrder.length; index++) {
        var id = String(this.filterOrder[index]);
        var definition = this.filterDefinitions[id];
        if (!definition) continue;
        this.store.Replace(
            "filter_" + id + "_default_bypass",
            this.numberOrDefault(definition.defaultBypass, 0)
        );
        this.store.SetParse("filters::" + id, JSON.stringify(definition.source));
    }
};

EqStorage.prototype.ParameterEntries = function(definition) {
    var names = definition.type === "gain" ? ["gain"] :
        (definition.type === "tilt" ? ["gain", "pivot"] : ["gain", "freq", "q"]);
    var result = [];
    for (var index = 0; index < names.length; index++) {
        var parameter = definition.parameters[names[index]];
        result.push({
            name: names[index],
            control: String(parameter.control || ""),
            scale: String(parameter.scale || "linear"),
            min: Number(parameter.min),
            max: Number(parameter.max)
        });
    }
    return result;
};

EqStorage.prototype.FindControlParameterIndex = function(definition, control) {
    var parameters = this.ParameterEntries(definition);
    for (var index = 0; index < parameters.length; index++) {
        if (parameters[index].control === control) return index;
    }
    return -1;
};

EqStorage.prototype.FindParameterIndex = function(parameters, name) {
    for (var index = 0; index < parameters.length; index++) {
        if (parameters[index].name === name) return index;
    }
    return -1;
};

EqStorage.prototype.FindFrequencyParameterIndex = function(parameters) {
    var index = this.FindParameterIndex(parameters, "freq");
    return index >= 0 ? index : this.FindParameterIndex(parameters, "pivot");
};

EqStorage.prototype.ClampParameter = function(parameter, value) {
    if (!isFinite(value)) value = parameter.min;
    return Math.max(parameter.min, Math.min(parameter.max, value));
};

EqStorage.prototype.Denormalize = function(parameter, value) {
    value = Math.max(0, Math.min(1, value));
    return parameter.scale === "logarithmic"
        ? parameter.min * Math.pow(parameter.max / parameter.min, value)
        : parameter.min + value * (parameter.max - parameter.min);
};

EqStorage.prototype.Normalize = function(parameter, value) {
    value = this.ClampParameter(parameter, Number(value));
    return parameter.scale === "logarithmic"
        ? Math.log(value / parameter.min) / Math.log(parameter.max / parameter.min)
        : (value - parameter.min) / (parameter.max - parameter.min);
};

EqStorage.prototype.ParameterValue = function(parameters, values, name, fallback) {
    var index = this.FindParameterIndex(parameters, name);
    return index >= 0 ? Number(values[index]) : fallback;
};

EqStorage.prototype.FrequencyValue = function(parameters, values) {
    var index = this.FindFrequencyParameterIndex(parameters);
    return index >= 0 ? Number(values[index]) : 1000;
};

EqStorage.prototype.ensureFilterInAllBanks = function(id) {
    var definition = this.filterDefinitions[String(id)];
    if (!definition || !(definition.defaultValues instanceof Array)) {
        return;
    }
    for (var row = 1; row <= this.bankCount(); row++) {
        if (!this.loadStoredFilterAtBank(id, row)) {
            this.saveStoredFilterAtBank(new BankFilter(
                id,
                definition.defaultValues,
                this.numberOrDefault(definition.defaultBypass, 0)
            ), row);
        }
    }
};

EqStorage.prototype.EnsureAllDefinedFiltersInBanks = function() {
    for (var index = 0; index < this.filterOrder.length; index++) {
        this.ensureFilterInAllBanks(this.filterOrder[index]);
    }
};

EqStorage.prototype.initializeBankDefaults = function(row) {
    for (var index = 0; index < this.filterOrder.length; index++) {
        var id = this.filterOrder[index];
        var definition = this.filterDefinitions[String(id)];
        if (definition && definition.defaultValues instanceof Array) {
            this.saveStoredFilterAtBank(new BankFilter(
                id,
                definition.defaultValues,
                this.numberOrDefault(definition.defaultBypass, 0)
            ), row);
        }
    }
};

EqStorage.prototype.restoreSelectedFilter = function(id) {
    this.PublishFilterState(id, this.selectedRow);
};

EqStorage.prototype.RequestStatePublish = function() {
    this.EnsureInitialized();
    this.store.RequestPublish();
};

EqStorage.prototype.PublishStateNow = function() {
    this.EnsureInitialized();
    this.store.PublishNow();
};

EqStorage.prototype.HandleStoreCommit = function(stateName, generation) {
    var payload = { stateName: stateName, generation: generation };
    this.sendEnvelope(3, MessageEnvelope.create(
        "device.state.changed", "eq.chain", payload, "eq.storage"
    ));
    this.sendEnvelope(5, MessageEnvelope.create(
        "device.state.changed", "analyzer", payload, "eq.storage"
    ));
    this.sendEnvelope(5, MessageEnvelope.create(
        "device.state.changed", "approximator", payload, "eq.storage"
    ));
    this.SchedulePersistence();
};

EqStorage.prototype.SchedulePersistence = function() {
    if (!this.persistenceReady) return;
    this.persistenceTask.cancel();
    this.persistenceTask.schedule(DeviceStateStore.PersistenceDelayMs);
};

EqStorage.prototype.commitState = function() {
    if (!this.persistenceReady) {
        return;
    }
    this.store.CommitRevision();
    this.isPersistingState = true;
    outlet(6, "dictionary", this.store.Name());
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
        if (this.isMissing(this.store.Get(key))) {
            this.store.Replace(key, this.generateBankName(i));
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
            var existingName = this.store.Get(this.bankNameKey(i));
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
    this.store.Replace(this.bankNameKey(to), this.store.Get(this.bankNameKey(from)));
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
    this.store.Remove(this.bankNameKey(index));
    for (var i = 0; i < this.filterOrder.length; i++) {
        this.store.Remove(this.filterPath(index, this.filterOrder[i]));
        this.store.Remove(this.bypassPath(index, this.filterOrder[i]));
    }
};

EqStorage.prototype.removeStoredFilterAtBank = function(id, bankIndex) {
    this.store.Remove(this.filterPath(bankIndex, id));
    this.store.Remove(this.bypassPath(bankIndex, id));
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
EqStorage.prototype.bankCount = function() { return this.numberOrDefault(this.store.Get("bank_count"), 0); };
EqStorage.prototype.selectedBankName = function() { return this.bankName(this.selectedRow); };
EqStorage.prototype.bankName = function(index) {
    var value = this.store.Get(this.bankNameKey(index));
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

function inletassist(index) {
    var descriptions = [
        "Commands: initialize, bang, add, remove, select, rename, delete",
        "message <dictionary type=filter.control|filter.set|filter.set_many|filter.reset|system.status>",
        "Persistence: dictionary <state> or persistence_ready"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    var descriptions = [
        "reserved",
        "Bank list commands: clear, append, setid",
        "Status and errors",
        "message <device.state.changed> to EqChain",
        "reserved",
        "message <filter.state|device.state.changed|system.status> to the message bus",
        "dictionary <complete persistent bank state>"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() {
    eqStorage.initializeStateModel();
    eqStorage.publishBankList();
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
