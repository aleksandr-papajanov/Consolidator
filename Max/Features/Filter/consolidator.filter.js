autowatch = 1;
inlets = 1;
outlets = 3;

include("../Shared/JS/DictionaryReader.js");
include("../Shared/JS/Messages/MessageEnvelope.js");

function Filter(slot) {
    this.slot = Number(slot) || 0;
    this.configurationName = "";
    this.definitionName = "";
    this.definition = null;
    this.parameters = [];
    this.values = [];
    this.defined = false;
    this.bypassed = false;
}

Filter.prototype.HandleEnvelope = function(dictionaryName) {
    var message = MessageEnvelope.fromMaxDictionary(dictionaryName);
    if (!message || message.target !== "filter") {
        return;
    }

    if (message.type === "filter.restore") {
        this.Restore(message.payload);
    }
    else if (message.type === "filter.apply") {
        this.Apply(message.payload);
    }
    else if (message.type === "filter.edit") {
        this.Edit(message.payload);
    }
};

Filter.prototype.Define = function(configurationName) {
    try {
        var configuration = new DictionaryReader(String(configurationName));
        var selectedSlot = this.Number(configuration.selected, this.slot);
        var definition = configuration.filters && configuration.filters[selectedSlot];
        if (!definition || this.Number(definition.slot, 0) !== this.slot) {
            throw new Error("missing_filter_configuration");
        }

        this.parameters = this.ReadParameters(definition);
        this.configurationName = String(configurationName);
        this.definition = new Dict();
        this.definition.clone(configuration.dict.name);
        this.definitionName = this.definition.name;
        this.values = this.DefaultValues();
        this.bypassed = false;
        this.defined = true;

        this.PublishDefinition();
        this.PublishValues();
        this.PublishStatus("ready");
        this.PublishEnvelope("system.status", "bus.hub", {
            feature: "filter",
            filterId: this.slot,
            state: "ready"
        });
    }
    catch (error) {
        this.PublishError("invalid_filter_configuration_dictionary");
    }
};

Filter.prototype.UpdateControl = function(control, value) {
    if (!this.defined) {
        return;
    }

    control = String(control || "");
    value = Number(value);
    if (!isFinite(value)) {
        this.PublishError("invalid_filter_control");
        return;
    }

    if (control === "bypass") {
        if (value !== 0 && value !== 1) {
            this.PublishError("bypass_must_be_0_or_1");
            return;
        }
        this.SetLocalBypass(value, true);
        return;
    }

    if (value < 0 || value > 1 || !this.SetNormalizedParameter(control, value)) {
        this.PublishError("unsupported_filter_control");
        return;
    }

    this.PublishChanged();
    this.PublishValues();
};

Filter.prototype.ResetState = function() {
    if (!this.defined) {
        return;
    }

    this.bypassed = false;
    this.values = this.DefaultValues();
    this.PublishChanged();
    this.PublishValues();
};

Filter.prototype.Restore = function(payload) {
    if (!this.IsTarget(payload) || !this.defined) {
        return;
    }

    if (!this.SetValues(payload.values)) {
        this.PublishError("invalid_global_filter_values");
        return;
    }

    this.bypassed = Number(payload.bypass) === 1;
    this.PublishValues();
};

Filter.prototype.Apply = function(payload) {
    if (!this.IsTarget(payload) || !this.defined) {
        return;
    }

    if (!this.SetValues(payload.values)) {
        this.PublishError("invalid_global_filter_values");
        return;
    }

    this.PublishChanged(payload.bankIndex);
    this.PublishValues();
};

Filter.prototype.SetLocalBypass = function(value, publish) {
    if (value !== 0 && value !== 1) {
        this.PublishError("bypass_must_be_0_or_1");
        return;
    }

    var bypassed = value === 1;
    if (this.bypassed === bypassed) {
        return;
    }

    this.bypassed = bypassed;
    if (publish) {
        this.PublishChanged();
    }
    this.PublishValues();
};

Filter.prototype.Edit = function(payload) {
    if (!this.IsTarget(payload) || !this.defined) {
        return;
    }

    if (payload.parameter === "q") {
        if (!this.SetAbsoluteParameter("q", Number(payload.value))) {
            this.PublishError("invalid_filter_edit");
            return;
        }
    }
    else {
        var frequency = Number(payload.frequency);
        var gain = Number(payload.gain);
        if (!isFinite(frequency) || !isFinite(gain) ||
            !this.SetGraphParameters(frequency, gain)) {
            this.PublishError("invalid_filter_edit");
            return;
        }
    }

    this.PublishChanged();
    this.PublishValues();
};

Filter.prototype.ReadParameters = function(definition) {
    var type = String(definition.type || "");
    var names = type === "gain" ? ["gain"] :
        (type === "tilt" ? ["gain", "pivot"] : ["gain", "freq", "q"]);
    var parameters = [];
    for (var index = 0; index < names.length; index++) {
        var source = definition.parameters && definition.parameters[names[index]];
        if (!source) {
            throw new Error("missing_filter_parameter");
        }

        var parameter = {
            name: names[index],
            control: String(source.control || ""),
            scale: String(source.scale || "linear"),
            min: this.Number(source.min, NaN),
            max: this.Number(source.max, NaN),
            defaultValue: this.Number(source.default, NaN),
            discreteValues: source.values instanceof Array ? source.values.map(Number) : []
        };
        this.ValidateParameter(parameter);
        parameters.push(parameter);
    }
    return parameters;
};

Filter.prototype.ValidateParameter = function(parameter) {
    if (!isFinite(parameter.defaultValue) ||
        (parameter.scale !== "linear" && parameter.scale !== "logarithmic" && parameter.scale !== "discrete")) {
        throw new Error("invalid_filter_parameter");
    }

    if (parameter.scale === "discrete") {
        if (parameter.discreteValues.length === 0) {
            throw new Error("invalid_discrete_parameter");
        }
        parameter.min = Math.min.apply(null, parameter.discreteValues);
        parameter.max = Math.max.apply(null, parameter.discreteValues);
        if (parameter.discreteValues.indexOf(parameter.defaultValue) < 0) {
            throw new Error("invalid_discrete_default");
        }
        return;
    }

    if (!isFinite(parameter.min) || !isFinite(parameter.max) ||
        parameter.max <= parameter.min || parameter.defaultValue < parameter.min ||
        parameter.defaultValue > parameter.max ||
        (parameter.scale === "logarithmic" && parameter.min <= 0)) {
        throw new Error("invalid_filter_range");
    }
};

Filter.prototype.DefaultValues = function() {
    var values = [];
    for (var index = 0; index < this.parameters.length; index++) {
        values.push(this.parameters[index].defaultValue);
    }
    return values;
};

Filter.prototype.SetValues = function(values) {
    if (!(values instanceof Array) || values.length !== this.parameters.length) {
        return false;
    }

    var nextValues = [];
    for (var index = 0; index < values.length; index++) {
        var value = Number(values[index]);
        if (!this.IsAbsoluteValueValid(this.parameters[index], value)) {
            return false;
        }
        nextValues.push(value);
    }
    this.values = nextValues;
    return true;
};

Filter.prototype.SetNormalizedParameter = function(control, value) {
    if (!isFinite(value) || value < 0 || value > 1) {
        return false;
    }

    for (var index = 0; index < this.parameters.length; index++) {
        var parameter = this.parameters[index];
        if (parameter.name === control ||
            (control === "frequency" && (parameter.name === "freq" || parameter.name === "pivot"))) {
            this.values[index] = this.Denormalize(parameter, value);
            return true;
        }
    }
    return false;
};

Filter.prototype.SetAbsoluteParameter = function(control, value) {
    for (var index = 0; index < this.parameters.length; index++) {
        var parameter = this.parameters[index];
        if (parameter.name === control && this.IsAbsoluteValueValid(parameter, value)) {
            this.values[index] = value;
            return true;
        }
    }
    return false;
};

Filter.prototype.SetGraphParameters = function(frequency, gain) {
    var changed = false;
    for (var index = 0; index < this.parameters.length; index++) {
        var parameter = this.parameters[index];
        if (parameter.name === "gain") {
            this.values[index] = this.ClampAbsolute(parameter, gain);
            changed = true;
        }
        else if (parameter.name === "freq" || parameter.name === "pivot") {
            this.values[index] = this.ClampAbsolute(parameter, frequency);
            changed = true;
        }
    }
    return changed;
};

Filter.prototype.Denormalize = function(parameter, value) {
    var normalized = Math.max(0, Math.min(1, Number(value)));
    if (parameter.scale === "logarithmic") {
        return parameter.min * Math.pow(parameter.max / parameter.min, normalized);
    }
    if (parameter.scale === "discrete") {
        var index = Math.round(normalized * (parameter.discreteValues.length - 1));
        return parameter.discreteValues[index];
    }
    return parameter.min + normalized * (parameter.max - parameter.min);
};

Filter.prototype.ClampAbsolute = function(parameter, value) {
    var clamped = Math.max(parameter.min, Math.min(parameter.max, Number(value)));
    if (parameter.scale !== "discrete") {
        return clamped;
    }
    var closest = parameter.discreteValues[0];
    for (var index = 1; index < parameter.discreteValues.length; index++) {
        if (Math.abs(parameter.discreteValues[index] - clamped) < Math.abs(closest - clamped)) {
            closest = parameter.discreteValues[index];
        }
    }
    return closest;
};

Filter.prototype.IsAbsoluteValueValid = function(parameter, value) {
    if (!isFinite(value) || value < parameter.min || value > parameter.max) {
        return false;
    }
    return parameter.scale !== "discrete" || parameter.discreteValues.indexOf(value) >= 0;
};

Filter.prototype.Normalize = function(parameter, value) {
    var number = Math.max(parameter.min, Math.min(parameter.max, Number(value)));
    if (parameter.scale === "logarithmic") {
        return Math.log(number / parameter.min) / Math.log(parameter.max / parameter.min);
    }
    if (parameter.scale === "discrete") {
        var closest = 0;
        for (var index = 1; index < parameter.discreteValues.length; index++) {
            if (Math.abs(parameter.discreteValues[index] - number) <
                Math.abs(parameter.discreteValues[closest] - number)) {
                closest = index;
            }
        }
        return parameter.discreteValues.length === 1 ? 0 :
            closest / (parameter.discreteValues.length - 1);
    }
    return (number - parameter.min) / (parameter.max - parameter.min);
};

Filter.prototype.PublishDefinition = function() {
    var payload = {
        filterId: this.slot,
        contractName: this.definitionName,
        defaultValues: this.DefaultValues(),
        defaultBypass: 0
    };
    this.PublishEnvelope("filter.define", "eq.storage", payload);
};

Filter.prototype.PublishChanged = function(bankIndex) {
    var payload = {
        filterId: this.slot,
        values: this.values.slice(0),
        bypass: this.bypassed ? 1 : 0
    };
    if (bankIndex !== undefined && bankIndex !== null) {
        payload.bankIndex = Number(bankIndex);
    }
    this.PublishEnvelope("filter.changed", "eq.storage", payload);
};

Filter.prototype.PublishEnvelope = function(type, target, payload) {
    var envelope = MessageEnvelope.create(type, target, payload, "filter");
    var dictionary = envelope.toMaxDictionary();
    outlet(0, "message", dictionary.name);
};

Filter.prototype.PublishValues = function() {
    var normalizedValues = [];
    for (var index = 0; index < this.values.length; index++) {
        normalizedValues.push(this.Normalize(this.parameters[index], this.values[index]));
    }
    outlet(1, ["status", "values"].concat(normalizedValues, [this.bypassed ? 1 : 0]));
};

Filter.prototype.PublishStatus = function(state) {
    outlet(1, "status", state);
};

Filter.prototype.PublishError = function(code) {
    outlet(2, "error", code);
};

Filter.prototype.IsTarget = function(payload) {
    return payload && Number(payload.filterId) === this.slot;
};

Filter.prototype.Number = function(value, fallback) {
    var number = Number(value);
    return isFinite(number) ? number : fallback;
};

var filter = new Filter(jsarguments[1]);

function inletassist(index) {
    assist(index === 0
        ? "Local: define <dictionary>, update <control> <0..1>, reset, slot <id>; bus: filter.restore|filter.apply|filter.edit envelope"
        : "");
}

function outletassist(index) {
    var descriptions = [
        "message <filter.define|filter.changed envelope>",
        "status ready or values <normalized parameters> <bypass>",
        "error <code>"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function slot(value) {
    filter.slot = Number(value) || 0;
}

function define(dictionaryName) {
    filter.Define(dictionaryName);
}

function update(control, value) {
    filter.UpdateControl(control, value);
}

function reset() {
    filter.ResetState();
}

function message(dictionaryName) {
    filter.HandleEnvelope(dictionaryName);
}
