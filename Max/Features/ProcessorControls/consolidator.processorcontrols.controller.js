autowatch = 1;
inlets = 2;
outlets = 5;

function ProcessorControlsController() {
    this.requestId = 0;
    this.selectedBankId = 1;
    this.filterDefinitions = {};
    this.processorDefinitions = {};
    this.controlValues = {};
}

ProcessorControlsController.prototype.SendCommand = function(name, fields) {
    this.requestId += 1;
    outlet(0, "command", [1, "processor.controls", this.requestId, name].concat(fields || []));
};

ProcessorControlsController.prototype.ReadParameter = function(values, position) {
    return {
        value: {
            name: String(values[position]),
            minimum: Number(values[position + 1]),
            maximum: Number(values[position + 2]),
            logarithmic: Number(values[position + 3]) === 1,
            defaultValue: Number(values[position + 4])
        },
        next: position + 5
    };
};

ProcessorControlsController.prototype.HandleFilterDefinitions = function(values) {
    var count = Number(values[5]);
    var position = 6;
    this.filterDefinitions = {};
    for (var index = 0; index < count; index++) {
        var id = Number(values[position++]);
        var section = String(values[position++]);
        var type = String(values[position++]);
        var defaultBypass = Number(values[position++]) !== 0;
        var parameterCount = Number(values[position++]);
        var parameters = [];
        for (var parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++) {
            var decoded = this.ReadParameter(values, position);
            parameters.push(decoded.value);
            position = decoded.next;
        }
        this.filterDefinitions[id] = {
            id: id,
            section: section,
            type: type,
            defaultBypass: defaultBypass,
            parameters: parameters
        };
    }
};

ProcessorControlsController.prototype.HandleProcessorDefinitions = function(values) {
    var count = Number(values[5]);
    var position = 6;
    this.processorDefinitions = {};
    for (var index = 0; index < count; index++) {
        var id = String(values[position++]);
        var parameterCount = Number(values[position++]);
        var parameters = [];
        for (var parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++) {
            var decoded = this.ReadParameter(values, position);
            parameters.push(decoded.value);
            position = decoded.next;
        }
        this.processorDefinitions[id] = { id: id, parameters: parameters };
    }
};

ProcessorControlsController.prototype.FindParameter = function(definition, name) {
    if (!definition) return null;
    for (var index = 0; index < definition.parameters.length; index++) {
        if (definition.parameters[index].name === name) return definition.parameters[index];
    }
    return null;
};

ProcessorControlsController.prototype.FindFilterParameter = function(definition, controlId) {
    if (controlId !== "frequency") return this.FindParameter(definition, controlId);
    return this.FindParameter(definition, "freq") || this.FindParameter(definition, "pivot");
};

ProcessorControlsController.prototype.FilterControlId = function(parameterName) {
    return parameterName === "freq" || parameterName === "pivot" ? "frequency" : parameterName;
};

ProcessorControlsController.prototype.ToAbsolute = function(definition, normalized) {
    var value = Math.max(0, Math.min(1, Number(normalized)));
    if (definition.logarithmic && definition.minimum > 0) {
        return definition.minimum * Math.pow(definition.maximum / definition.minimum, value);
    }
    return definition.minimum + value * (definition.maximum - definition.minimum);
};

ProcessorControlsController.prototype.ToNormalized = function(definition, absolute) {
    var value = Number(absolute);
    if (definition.logarithmic && definition.minimum > 0) {
        return Math.log(value / definition.minimum) / Math.log(definition.maximum / definition.minimum);
    }
    return (value - definition.minimum) / (definition.maximum - definition.minimum);
};

ProcessorControlsController.prototype.HandleLocal = function(values) {
    if (values.length < 2) return;
    var device = String(values[0]);
    if (device === "preeq" || device === "posteq") {
        var eqAction = String(values[1]);
        var section = device === "preeq" ? "pre" : "post";
        if (eqAction === "reset") {
            this.SendCommand("eq.reset_section", [this.selectedBankId, section]);
        } else if (eqAction === "bypass") {
            var eqBypass = Number(values[2]) ? 1 : 0;
            this.SendCommand("eq.set_section_bypass", [this.selectedBankId, section, eqBypass]);
        }
        return;
    }
    if (device === "filter") {
        var filterId = Number(values[1]);
        var parameterName = String(values[2]);
        if (parameterName === "reset") {
            this.SendCommand("eq.reset_filter", [this.selectedBankId, filterId]);
            return;
        }
        if (parameterName === "bypass") {
            var bypassValue = Number(values[3]) ? 1 : 0;
            this.RememberValue(["filter", filterId, "bypass", bypassValue]);
            this.SendCommand("eq.set_bypass", [this.selectedBankId, filterId, bypassValue]);
            return;
        }
        var parameter = this.FindFilterParameter(this.filterDefinitions[filterId], parameterName);
        if (parameter) {
            var normalizedValue = Math.max(0, Math.min(1, Number(values[3])));
            this.RememberValue(["filter", filterId, parameterName, normalizedValue]);
            this.SendCommand("eq.set_parameter", [
                this.selectedBankId,
                filterId,
                parameter.name,
                this.ToAbsolute(parameter, normalizedValue)
            ]);
        }
        return;
    }

    if (device === "input_gain" || device === "output_gain") {
        var gainDefinition = this.processorDefinitions[device];
        var gainParameter = this.FindParameter(gainDefinition, "gain");
        if (!gainParameter) return;
        var gainValue = Math.max(0, Math.min(1, Number(values[1])));
        this.RememberValue([device, "gain", gainValue]);
        this.SendCommand("gain.set_parameter", [
            device === "input_gain" ? "input" : "output",
            this.ToAbsolute(gainParameter, gainValue)
        ]);
        return;
    }

    var name = String(values[1]);
    if (name === "reset") {
        this.SendCommand(device + ".reset", []);
        return;
    }
    if (name === "bypass") {
        var processorBypass = Number(values[2]) ? 1 : 0;
        this.SendCommand(device + ".set_bypass", [processorBypass]);
        return;
    }
    var definition = this.processorDefinitions[device];
    var processorParameter = this.FindParameter(definition, name);
    if (!processorParameter) return;
    var processorValue = Math.max(0, Math.min(1, Number(values[2])));
    this.RememberValue([device, name, processorValue]);
    var absolute = this.ToAbsolute(processorParameter, processorValue);
    this.SendCommand(device + ".set_parameter", device === "saturator"
        ? [absolute]
        : [name, absolute]);
};

ProcessorControlsController.prototype.ControlVarName = function(fields) {
    if (String(fields[0]) === "filter") {
        return "filter." + Number(fields[1]) + "." + String(fields[2]);
    }
    return String(fields[0]) + "." + String(fields[1]);
};

ProcessorControlsController.prototype.RememberValue = function(fields) {
    this.controlValues[this.ControlVarName(fields)] = Number(fields[fields.length - 1]);
};

ProcessorControlsController.prototype.SendValue = function(fields) {
    var value = fields[fields.length - 1];
    var varName = this.ControlVarName(fields);
    var previous = this.controlValues[varName];
    if (previous !== undefined && Math.abs(previous - value) <= 0.0000001) return;
    this.controlValues[varName] = value;
    outlet(1, ["script", "sendbox", varName, "set", value]);
};

ProcessorControlsController.prototype.HandleEqSnapshot = function(values) {
    this.selectedBankId = Number(values[5]);
    var position = 7;
    var bankCount = Number(values[6]);
    for (var bankIndex = 0; bankIndex < bankCount; bankIndex++) {
        var bankId = Number(values[position++]);
        position++;
        var preBypass = Number(values[position++]);
        var postBypass = Number(values[position++]);
        var filterCount = Number(values[position++]);
        if (bankId === this.selectedBankId) {
            this.SendValue(["preeq", "bypass", preBypass]);
            this.SendValue(["posteq", "bypass", postBypass]);
        }
        for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
            var filterId = Number(values[position++]);
            var bypass = Number(values[position++]);
            var valueCount = Number(values[position++]);
            var definition = this.filterDefinitions[filterId];
            if (bankId === this.selectedBankId && definition) {
                for (var valueIndex = 0; valueIndex < valueCount; valueIndex++) {
                    var parameter = definition.parameters[valueIndex];
                    this.SendValue([
                        "filter", filterId, this.FilterControlId(parameter.name),
                        this.ToNormalized(parameter, values[position + valueIndex])
                    ]);
                }
                this.SendValue(["filter", filterId, "bypass", bypass]);
            }
            position += valueCount;
        }
    }
};

ProcessorControlsController.prototype.HandleDspSnapshot = function(values) {
    var count = values.length;
    if (count < 8 || !this.processorDefinitions.input_gain ||
        !this.processorDefinitions.compressor || !this.processorDefinitions.saturator ||
        !this.processorDefinitions.output_gain) return;
    var inputGain = this.processorDefinitions.input_gain;
    var compressor = this.processorDefinitions.compressor;
    var saturator = this.processorDefinitions.saturator;
    var outputGain = this.processorDefinitions.output_gain;
    outlet(3, "compressor_state", [
        Number(values[count - 7]),
        Number(values[count - 6]),
        Number(values[count - 5]),
        Number(values[count - 4])
    ]);
    outlet(4, "saturator_state", [Number(values[count - 3]), Number(values[count - 2])]);
    this.SendValue(["input_gain", "gain", this.ToNormalized(this.FindParameter(inputGain, "gain"), values[count - 8])]);
    this.SendValue(["compressor", "bypass", Number(values[count - 7])]);
    this.SendValue(["compressor", "attack", this.ToNormalized(this.FindParameter(compressor, "attack"), values[count - 6])]);
    this.SendValue(["compressor", "release", this.ToNormalized(this.FindParameter(compressor, "release"), values[count - 5])]);
    this.SendValue(["compressor", "threshold", this.ToNormalized(this.FindParameter(compressor, "threshold"), values[count - 4])]);
    this.SendValue(["saturator", "bypass", Number(values[count - 3])]);
    this.SendValue(["saturator", "saturation", this.ToNormalized(this.FindParameter(saturator, "saturation"), values[count - 2])]);
    this.SendValue(["output_gain", "gain", this.ToNormalized(this.FindParameter(outputGain, "gain"), values[count - 1])]);
};

ProcessorControlsController.prototype.HandleSnapshot = function(values) {
    if (values.length < 6 || String(values[0]) !== "snapshot") return;
    var store = String(values[3]);
    if (store === "definitions") this.HandleFilterDefinitions(values);
    else if (store === "processor_definitions") this.HandleProcessorDefinitions(values);
    else if (store === "eq") this.HandleEqSnapshot(values);
    else if (store === "dsp") this.HandleDspSnapshot(values);
};

var controller = new ProcessorControlsController();

function inletassist(index) {
    assist([
        "Normalized local input: preeq|posteq bypass <0|1>, preeq|posteq reset, filter <id> <parameter|bypass> <0..1>, filter <id> reset, input_gain <0..1>, compressor <attack|release|threshold|bypass> <0..1>, saturator <saturation|bypass> <0..1>, output_gain <0..1>",
        "Host input: definitions, processor_definitions, EQ, and DSP snapshots; events are ignored"
    ][index] || "");
}

function outletassist(index) {
    assist([
        "Host commands: eq.*, gain.set_parameter, compressor.*, saturator.*",
        "thispatcher commands: script sendbox <stable-varname> set <normalized-value>",
        "Diagnostics: error <code>",
        "Compressor visual state: compressor_state <bypass> <attackMs> <releaseMs> <thresholdDb>",
        "Saturator visual state: saturator_state <bypass> <saturation>"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function filter() { if (inlet === 0) controller.HandleLocal(["filter"].concat(arrayfromargs(arguments))); }
function preeq() { if (inlet === 0) controller.HandleLocal(["preeq"].concat(arrayfromargs(arguments))); }
function posteq() { if (inlet === 0) controller.HandleLocal(["posteq"].concat(arrayfromargs(arguments))); }
function input_gain() { if (inlet === 0) controller.HandleLocal(["input_gain"].concat(arrayfromargs(arguments))); }
function compressor() { if (inlet === 0) controller.HandleLocal(["compressor"].concat(arrayfromargs(arguments))); }
function saturator() { if (inlet === 0) controller.HandleLocal(["saturator"].concat(arrayfromargs(arguments))); }
function output_gain() { if (inlet === 0) controller.HandleLocal(["output_gain"].concat(arrayfromargs(arguments))); }
function snapshot() { if (inlet === 1) controller.HandleSnapshot(["snapshot"].concat(arrayfromargs(arguments))); }
function event() {}
function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 1 && values.length && String(values[0]) === "snapshot") controller.HandleSnapshot(values);
}
