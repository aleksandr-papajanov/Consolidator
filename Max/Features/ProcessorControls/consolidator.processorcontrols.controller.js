autowatch = 1;
inlets = 3;
outlets = 3;
include("../Shared/JS/TargetLevelIndicator.js");
include("../Shared/JS/SaturationVisualization.js");

function ProcessorControlsController() {
    this.requestId = 0;
    this.selectedBankId = 1;
    this.filterDefinitions = {};
    this.processorDefinitions = {};
    this.controlValues = {};
    this.processorControlStates = {
        compressor: { bypass: null, listen: null },
        saturator: { bypass: null, listen: null }
    };
    this.visualDevice = jsarguments.length > 1 ? String(jsarguments[1]) : "";
    this.outputLevelIndicator = new TargetLevelIndicator();
    this.saturationVisualization = new SaturationVisualization();
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
            type: type,
            defaultBypass: defaultBypass,
            parameters: parameters
        };
    }
    for (var filterId in this.filterDefinitions) {
        if (!this.filterDefinitions.hasOwnProperty(filterId)) continue;
        if (!this.FindParameter(this.filterDefinitions[filterId], "q")) {
            outlet(1, ["script", "sendbox", "filter." + filterId + ".q", "active", 0]);
        }
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
    if (!definition) return 0;
    var value = Math.max(0, Math.min(1, Number(normalized)));
    if (definition.logarithmic && definition.minimum > 0) {
        return definition.minimum * Math.pow(definition.maximum / definition.minimum, value);
    }
    return definition.minimum + value * (definition.maximum - definition.minimum);
};

ProcessorControlsController.prototype.ToNormalized = function(definition, absolute) {
    if (!definition) return 0;
    var value = Number(absolute);
    if (definition.logarithmic && definition.minimum > 0) {
        return Math.log(value / definition.minimum) / Math.log(definition.maximum / definition.minimum);
    }
    return (value - definition.minimum) / (definition.maximum - definition.minimum);
};

ProcessorControlsController.prototype.HandleLocal = function(values) {
    if (values.length < 2) return;
    var device = String(values[0]);
    if (device === "eq") {
        var eqAction = String(values[1]);
        if (eqAction === "reset") {
            this.SendCommand("eq.reset", [this.selectedBankId]);
        } else if (eqAction === "bypass") {
            var eqBypass = Number(values[2]) ? 1 : 0;
            this.SendCommand("eq.set_chain_bypass", [this.selectedBankId, eqBypass]);
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

    if (device === "compressor" || device === "saturator") {
        var processorAction = String(values[1]);
        if (processorAction === "input-output" && values.length >= 4) {
            var inputOutputIndex = Number(values[2]);
            var inputOutputValue = Math.max(0, Math.min(1, Number(values[3])));
            if (inputOutputIndex !== 1 && inputOutputIndex !== 2) return;
            var inputOutputParameter = inputOutputIndex === 1 ? "input" : "output";
            var inputOutputDefinition = this.FindParameter(
                this.processorDefinitions[device], inputOutputParameter);
            if (!inputOutputDefinition) {
                outlet(2, "error", "missing_processor_definition", device, inputOutputParameter);
                return;
            }
            this.SendCommand(device + ".set_parameter", [
                inputOutputParameter,
                this.ToAbsolute(inputOutputDefinition, inputOutputValue)
            ]);
            return;
        }
        if (processorAction === "attack-release" && values.length >= 4) {
            var attackReleaseIndex = Number(values[2]);
            var attackReleaseValue = Math.max(0, Math.min(1, Number(values[3])));
            if (device !== "compressor" || (attackReleaseIndex !== 1 && attackReleaseIndex !== 2)) return;
            var attackReleaseParameter = attackReleaseIndex === 1 ? "attack" : "release";
            var attackReleaseDefinition = this.FindParameter(this.processorDefinitions.compressor, attackReleaseParameter);
            if (!attackReleaseDefinition) return;
            this.SendCommand("compressor.set_parameter", [
                attackReleaseParameter, this.ToAbsolute(attackReleaseDefinition, attackReleaseValue)
            ]);
            return;
        }
        if (processorAction === "detector" && values.length >= 5) {
            var detectorId = Number(values[2]);
            var detectorParameter = String(values[3]);
            if (isFinite(Number(values[3]))) {
                detectorParameter = ["gain", "frequency", "q"][Math.max(0, Math.min(2, Number(values[3]) - 1))];
            }
            var detectorDefinition = this.FindParameter(
                this.processorDefinitions[device],
                "detector." + detectorId + "." + detectorParameter
            );
            var detectorValue = detectorParameter === "bypass"
                ? (Number(values[4]) ? 1 : 0)
                : (detectorDefinition ? this.ToAbsolute(detectorDefinition, values[4]) : null);
            if (detectorId >= 1 && detectorId <= 2 && detectorValue !== null) {
                this.SendCommand(device + ".set_detector_parameter", [
                    detectorId, detectorParameter, detectorValue
                ]);
                this.RememberValue([device, "detector", detectorId, detectorParameter, Number(values[4])]);
            }
            return;
        }
        if (processorAction === "mode") {
            this.SendCommand(device + ".set_mode", [Math.max(0, Math.floor(Number(values[2])) - 1)]);
            return;
        }
        if (processorAction === "control") {
            var controlIndex = Number(values[2]);
            var controlValue = values.length > 3 ? Number(values[3]) : 1;
            if (controlIndex === 1) this.SendCommand(device + ".set_bypass", [controlValue ? 1 : 0]);
            else if (controlIndex === 2 && controlValue) this.SendCommand(device + ".reset", []);
            else if (controlIndex === 3) this.SendCommand(device + ".set_detector_listen", [controlValue ? 1 : 0]);
            return;
        }
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
    if (!processorParameter) {
        outlet(2, "error", "missing_processor_definition", device, name);
        return;
    }
    var processorValue = Math.max(0, Math.min(1, Number(values[2])));
    this.RememberValue([device, name, processorValue]);
    var absolute = this.ToAbsolute(processorParameter, processorValue);
    this.SendCommand(device + ".set_parameter", [name, absolute]);
};

ProcessorControlsController.prototype.ControlVarName = function(fields) {
    if (String(fields[1]) === "input-output") return String(fields[0]) + ".inputOutput";
    if (String(fields[1]) === "attack-release") return String(fields[0]) + ".attackRelease";
    if (String(fields[1]) === "detector") {
        return String(fields[0]) + ".detector." + Number(fields[2]);
    }
    if (String(fields[0]) === "filter") {
        return "filter." + Number(fields[1]) + "." + String(fields[2]);
    }
    return String(fields[0]) + "." + String(fields[1]);
};

ProcessorControlsController.prototype.RememberValue = function(fields) {
    this.controlValues[this.ControlStateKey(fields)] = Number(fields[fields.length - 1]);
};

ProcessorControlsController.prototype.ControlStateKey = function(fields) {
    var varName = this.ControlVarName(fields);
    if (String(fields[1]) === "input-output" || String(fields[1]) === "attack-release") {
        return varName + "." + Number(fields[2]);
    }
    if (String(fields[1]) === "detector") {
        var detectorParameter = String(fields[3]);
        var detectorRing = { gain: 1, frequency: 2, q: 3 }[detectorParameter];
        return varName + "." + (detectorRing || detectorParameter);
    }
    return varName;
};

ProcessorControlsController.prototype.SendValue = function(fields) {
    var value = fields[fields.length - 1];
    var varName = this.ControlVarName(fields);
    var previous = this.controlValues[this.ControlStateKey(fields)];
    if (previous !== undefined && Math.abs(previous - value) <= 0.0000001) return;
    this.controlValues[this.ControlStateKey(fields)] = value;
    if (String(fields[1]) === "mix") {
        outlet(1, ["script", "sendbox", varName, "setValue", value]);
        return;
    }
    if (String(fields[1]) === "detector") {
        var ringIndex = { gain: 1, frequency: 2, q: 3 }[String(fields[3])];
        if (ringIndex) {
            outlet(1, ["script", "sendbox", varName, "set", ringIndex, value]);
            return;
        }
    }
    if (String(fields[1]) === "input-output" || String(fields[1]) === "attack-release") {
        outlet(1, ["script", "sendbox", varName, "set", Number(fields[2]), value]);
        return;
    }
    outlet(1, ["script", "sendbox", varName, "set", value]);
};

ProcessorControlsController.prototype.SendProcessorControlState = function(device, bypass, listen) {
    var state = this.processorControlStates[device];
    if (!state) return;
    var nextBypass = Number(bypass) ? 1 : 0;
    var nextListen = Number(listen) ? 1 : 0;
    if (state.bypass === nextBypass && state.listen === nextListen) return;
    state.bypass = nextBypass;
    state.listen = nextListen;
    var selection = [];
    if (state.bypass) selection.push(1);
    if (state.listen) selection.push(3);
    if (!selection.length) selection.push(0);
    outlet(1, ["script", "sendbox", device + ".control", "selection"].concat(selection));
};

ProcessorControlsController.prototype.HandleEqSnapshot = function(values) {
    this.selectedBankId = Number(values[5]);
    var position = 7;
    var bankCount = Number(values[6]);
    for (var bankIndex = 0; bankIndex < bankCount; bankIndex++) {
        var bankId = Number(values[position++]);
        position++;
        var bypass = Number(values[position++]);
        position++;
        var filterCount = Number(values[position++]);
        if (bankId === this.selectedBankId) {
            this.SendValue(["eq", "bypass", bypass]);
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
    if (count < 32 || !this.processorDefinitions.input_gain ||
        !this.processorDefinitions.compressor || !this.processorDefinitions.saturator ||
        !this.processorDefinitions.output_gain) return;
    var inputGain = this.processorDefinitions.input_gain;
    var compressor = this.processorDefinitions.compressor;
    var saturator = this.processorDefinitions.saturator;
    var outputGain = this.processorDefinitions.output_gain;
    var inputGainNormalized = this.ToNormalized(this.FindParameter(inputGain, "gain"), values[count - 32]);
    var outputGainNormalized = this.ToNormalized(this.FindParameter(outputGain, "gain"), values[count - 1]);
    this.SendValue(["input_gain", "gain", inputGainNormalized]);
    var compressorBypass = Number(values[count - 31]);
    this.SendValue(["compressor", "attack-release", 1, this.ToNormalized(this.FindParameter(compressor, "attack"), values[count - 30])]);
    this.SendValue(["compressor", "attack-release", 2, this.ToNormalized(this.FindParameter(compressor, "release"), values[count - 29])]);
    this.SendValue(["compressor", "input-output", 1, this.ToNormalized(this.FindParameter(compressor, "input"), values[count - 28])]);
    this.SendValue(["compressor", "input-output", 2, this.ToNormalized(this.FindParameter(compressor, "output"), values[count - 27])]);
    this.SendValue(["compressor", "mix", Number(values[count - 26])]);
    this.SendValue(["compressor", "mode", Number(values[count - 25]) + 1]);
    this.SendDetectorSnapshot("compressor", values, count - 24);
    var compressorListen = Number(values[count - 16]) !== 0;
    var saturatorBypass = Number(values[count - 15]);
    this.SendValue(["saturator", "input-output", 1,
        this.ToNormalized(this.FindParameter(saturator, "input"), values[count - 14])]);
    this.SendValue(["saturator", "input-output", 2,
        this.ToNormalized(this.FindParameter(saturator, "output"), values[count - 13])]);
    this.SendValue(["saturator", "mix", Number(values[count - 12])]);
    this.SendValue(["saturator", "mode", Number(values[count - 11]) + 1]);
    this.SendDetectorSnapshot("saturator", values, count - 10);
    var saturatorListen = Number(values[count - 2]) !== 0;
    this.SendProcessorControlState("compressor", compressorBypass, compressorListen);
    this.SendProcessorControlState("saturator", saturatorBypass, saturatorListen);
    this.SendValue(["output_gain", "gain", outputGainNormalized]);
};

ProcessorControlsController.prototype.SendDetectorSnapshot = function(device, values, position) {
    for (var index = 0; index < 2; index++) {
        var base = position + index * 4;
        this.SendValue([device, "detector", index + 1, "bypass", Number(values[base])]);
        this.SendValue([device, "detector", index + 1, "gain", this.NormalizeDetector(device, index + 1, "gain", values[base + 1])]);
        this.SendValue([device, "detector", index + 1, "frequency", this.NormalizeDetector(device, index + 1, "frequency", values[base + 2])]);
        this.SendValue([device, "detector", index + 1, "q", this.NormalizeDetector(device, index + 1, "q", values[base + 3])]);
        this.SendDetectorCurve(
            device,
            index + 1,
            values[base],
            values[base + 1],
            values[base + 2],
            values[base + 3]
        );
    }
};

ProcessorControlsController.prototype.SendDetectorCurve = function(
    device,
    filterId,
    bypass,
    gainDb,
    frequencyHz,
    q
) {
    if (device !== this.visualDevice) return;
    outlet(1, [
        "script",
        "sendbox",
        device + ".detectorCurve",
        "detector",
        filterId,
        Number(bypass) ? 1 : 0,
        Number(gainDb),
        Number(frequencyHz),
        Number(q)
    ]);
};

ProcessorControlsController.prototype.NormalizeDetector = function(device, filterId, name, value) {
    var definition = this.FindParameter(
        this.processorDefinitions[device], "detector." + filterId + "." + name
    );
    return definition ? this.ToNormalized(definition, value) : 0;
};

ProcessorControlsController.prototype.HandleSnapshot = function(values) {
    if (values.length < 6 || String(values[0]) !== "snapshot") return;
    var store = String(values[3]);
    if (store === "definitions") this.HandleFilterDefinitions(values);
    else if (store === "processor_definitions") this.HandleProcessorDefinitions(values);
    else if (store === "eq") this.HandleEqSnapshot(values);
    else if (store === "dsp") this.HandleDspSnapshot(values);
};

ProcessorControlsController.prototype.SendOutputLevelIndicator = function() {
    if (this.visualDevice !== "compressor" && this.visualDevice !== "saturator") return;
    this.SendDialVisualization(2, "signed", this.outputLevelIndicator.Value());
};

ProcessorControlsController.prototype.SendDialVisualization = function(ring, mode, value) {
    if (this.visualDevice !== "compressor" && this.visualDevice !== "saturator") return;
    outlet(1, [
        "script",
        "sendbox",
        this.visualDevice + ".inputOutput",
        "visualization",
        ring,
        mode,
        value
    ]);
};

ProcessorControlsController.prototype.HandleTargetLevel = function(value) {
    this.outputLevelIndicator.SetTargetDb(value);
    this.SendOutputLevelIndicator();
};

ProcessorControlsController.prototype.HandleProcessorTelemetry = function(values) {
    if (values.length < 9) return;
    var outputIndex = this.visualDevice === "compressor"
        ? 7
        : (this.visualDevice === "saturator" ? 8 : -1);
    if (outputIndex < 0) return;
    this.outputLevelIndicator.SetLevelDb(values[outputIndex]);
    this.SendOutputLevelIndicator();
    if (this.visualDevice === "compressor") {
        var reduction = Math.max(0.0, Math.min(1.0, -Number(values[0]) / 24.0));
        this.SendDialVisualization(2, "relative", -reduction);
    } else {
        var saturation = this.saturationVisualization.Update(values[1]);
        this.SendDialVisualization(1, "color", saturation);
    }
};

var controller = new ProcessorControlsController();

function inletassist(index) {
    assist([
        "Normalized local input: eq..., filter..., input_gain, output_gain, compressor attack-release/input-output <1|2> <0..1>, compressor mix/mode/bypass, compressor detector <1|2> <gain|frequency|q> <0..1>, saturator input-output <1|2> <0..1>, saturator mix/mode/bypass, saturator detector <1|2> <gain|frequency|q> <0..1>",
        "Host input: definitions, processor_definitions, EQ, and DSP snapshots; events are ignored",
        "UI telemetry: target_level <absoluteDb>, processor_telemetry <9 values>"
    ][index] || "");
}

function outletassist(index) {
    assist([
        "Host commands: eq.*, gain.set_parameter, compressor.*, saturator.*",
        "thispatcher commands: script sendbox <stable-varname> set <normalized-value>",
        "Diagnostics: error <code>"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function filter() { if (inlet === 0) controller.HandleLocal(["filter"].concat(arrayfromargs(arguments))); }
function eq() { if (inlet === 0) controller.HandleLocal(["eq"].concat(arrayfromargs(arguments))); }
function input_gain() { if (inlet === 0) controller.HandleLocal(["input_gain"].concat(arrayfromargs(arguments))); }
function compressor() { if (inlet === 0) controller.HandleLocal(["compressor"].concat(arrayfromargs(arguments))); }
function saturator() { if (inlet === 0) controller.HandleLocal(["saturator"].concat(arrayfromargs(arguments))); }
function output_gain() { if (inlet === 0) controller.HandleLocal(["output_gain"].concat(arrayfromargs(arguments))); }
function snapshot() { if (inlet === 1) controller.HandleSnapshot(["snapshot"].concat(arrayfromargs(arguments))); }
function event() {}
function status() {}
function target_level(value) {
    if (inlet === 2) controller.HandleTargetLevel(value);
}
function processor_telemetry() {
    if (inlet === 2) controller.HandleProcessorTelemetry(arrayfromargs(arguments));
}
function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 1 && values.length && String(values[0]) === "snapshot") controller.HandleSnapshot(values);
}
