autowatch = 1;
inlets = 1;
outlets = 3;

function FilterEndpoint(slot) {
    this.filterId = Number(slot) || 0;
    this.selectedBankId = 1;
    this.requestId = 0;
    this.configured = false;
    this.hasDefinition = false;
    this.hasState = false;
    this.readySent = false;
    this.latestValues = null;
}

FilterEndpoint.prototype.SendCommand = function(name, values) {
    this.requestId += 1;
    outlet(0, "command", [1, "filter." + this.filterId, this.requestId, name]
        .concat(values || []));
};

FilterEndpoint.prototype.Configure = function() {
    this.configured = true;
    this.hasDefinition = false;
    this.hasState = false;
    this.readySent = false;
    this.latestValues = null;
    this.SendCommand("component.attach", [1000 + this.filterId, "filter"]);
};

FilterEndpoint.prototype.PublishReady = function() {
    if (this.hasDefinition && this.hasState && !this.readySent) {
        this.readySent = true;
        outlet(1, "status", "ready");
    }
};

FilterEndpoint.prototype.Update = function(parameter, value) {
    if (!this.configured) return;
    var absolute = Number(value);
    if (!isFinite(absolute)) return outlet(2, "error", "invalid_filter_value");
    if (String(parameter) === "bypass") {
        this.SendCommand("eq.set_bypass", [this.selectedBankId, this.filterId, absolute >= 0.5 ? 1 : 0]);
    }
    else {
        this.SendCommand("eq.set_parameter", [this.selectedBankId, this.filterId, String(parameter), absolute]);
    }
};

FilterEndpoint.prototype.Reset = function() {
    if (this.configured) this.SendCommand("eq.reset_filter", [this.selectedBankId, this.filterId]);
};

FilterEndpoint.prototype.HandleSnapshot = function(values) {
    if (!this.configured) return;
    if (values.length < 5 || String(values[0]) !== "snapshot" || Number(values[1]) !== 1 ||
        String(values[2]) !== "host") return;
    var store = String(values[3]);
    if (store === "definitions") return this.HandleDefinitions(values);
    if (store !== "eq" || values.length < 7) return;
    var selectedBank = Number(values[5]);
    var bankCount = Number(values[6]);
    if (!isFinite(selectedBank) || !isFinite(bankCount) || bankCount < 1) {
        return outlet(2, "error", "invalid_eq_snapshot");
    }
    this.selectedBankId = selectedBank;
    var position = 7;
    var selectedValues = null;
    for (var bankIndex = 0; bankIndex < bankCount; bankIndex++) {
        if (position + 2 >= values.length) return outlet(2, "error", "invalid_eq_snapshot");
        var bankId = Number(values[position++]);
        position++;
        var filterCount = Number(values[position++]);
        if (!isFinite(bankId) || bankId < 1 || !isFinite(filterCount) || filterCount < 0 ||
            Math.floor(filterCount) !== filterCount) return outlet(2, "error", "invalid_eq_snapshot");
        for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
            if (position + 2 >= values.length) return outlet(2, "error", "invalid_eq_snapshot");
            var filterId = Number(values[position++]);
            var bypass = Number(values[position++]) ? 1 : 0;
            var valueCount = Number(values[position++]);
            if (!isFinite(filterId) || filterId < 1 || !isFinite(valueCount) || valueCount < 0 ||
                Math.floor(valueCount) !== valueCount || position + valueCount > values.length) {
                return outlet(2, "error", "invalid_eq_snapshot");
            }
            if (bankId === selectedBank && filterId === this.filterId) {
                selectedValues = [];
                for (var valueIndex = 0; valueIndex < valueCount; valueIndex++) {
                    selectedValues.push(Number(values[position + valueIndex]));
                }
                selectedValues.push(bypass);
            }
            position += valueCount;
        }
    }
    if (position !== values.length) return outlet(2, "error", "invalid_eq_snapshot");
    if (!selectedValues) return outlet(2, "error", "filter_state_not_found");
    this.hasState = true;
    this.latestValues = selectedValues;
    if (this.hasDefinition) {
        outlet(1, "status", ["values"].concat(this.latestValues));
    }
    this.PublishReady();
};

FilterEndpoint.prototype.HandleDefinitions = function(values) {
    if (values.length < 6) return outlet(2, "error", "invalid_definitions_snapshot");
    var filterCount = Number(values[5]);
    if (!isFinite(filterCount) || filterCount < 1 || Math.floor(filterCount) !== filterCount) {
        return outlet(2, "error", "invalid_definitions_snapshot");
    }
    var position = 6;
    for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
        if (position + 3 >= values.length) return outlet(2, "error", "invalid_definitions_snapshot");
        var filterId = Number(values[position++]);
        var type = String(values[position++]);
        var defaultBypass = Number(values[position++]) ? 1 : 0;
        var parameterCount = Number(values[position++]);
        if (!isFinite(filterId) || filterId < 1 || !isFinite(parameterCount) || parameterCount < 1 ||
            Math.floor(parameterCount) !== parameterCount) {
            return outlet(2, "error", "invalid_definitions_snapshot");
        }
        var definition = ["status", "definition", type, defaultBypass, parameterCount];
        for (var parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++) {
            if (position + 4 >= values.length) return outlet(2, "error", "invalid_definitions_snapshot");
            var name = String(values[position++]);
            var minimum = Number(values[position++]);
            var maximum = Number(values[position++]);
            var scale = Number(values[position++]) === 1 ? "logarithmic" : "linear";
            var defaultValue = Number(values[position++]);
            if (filterId === this.filterId) {
                definition = definition.concat([name, minimum, maximum, scale, defaultValue]);
            }
        }
        if (filterId === this.filterId) {
            this.hasDefinition = true;
            outlet(1, "status", definition.slice(1));
            if (this.hasState) {
                outlet(1, "status", ["values"].concat(this.latestValues));
            }
            this.PublishReady();
            return;
        }
    }
    outlet(2, "error", "filter_definition_not_found");
};

var endpoint = new FilterEndpoint(jsarguments[1]);

function inletassist(index) {
    assist(index === 0
        ? "Local: configure, update <parameter> <absolute>, reset; Host: snapshot; event is ignored"
        : "");
}
function outletassist(index) {
    assist([
        "Host commands: component.attach, eq.set_parameter, eq.set_bypass, eq.reset_filter",
        "Direct status: definition ..., ready, or values <absolute parameters> <bypass>",
        "Diagnostics: error <code>"
    ][index] || "");
}
setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function configure() { endpoint.Configure(); }
function update(parameter, value) { endpoint.Update(parameter, value); }
function reset() { endpoint.Reset(); }
function snapshot() { endpoint.HandleSnapshot(["snapshot"].concat(arrayfromargs(arguments))); }
function event() {}
function list() {
    var values = arrayfromargs(arguments);
    if (values.length && String(values[0]) === "snapshot") endpoint.HandleSnapshot(values);
}
