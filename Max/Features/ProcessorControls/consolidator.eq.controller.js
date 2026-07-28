autowatch = 1;
inlets = 2;
outlets = 4;
include("../Shared/JS/LatestValueDispatcher.js");

function EqController() {
    this.requestId = 0;
    this.selectedBankId = 1;
    this.filterDefinitions = {};
    this.pendingDialValues = {};
    this.linkColor = null;
    this.pendingFilterLimits = {};
    this.parameterDispatcher = new LatestValueDispatcher(
        16, this.FlushParameterUpdate, this);
}

EqController.prototype.SendCommand = function(name, fields) {
    this.requestId += 1;
    outlet(0, "command", [1, "eq.controls", this.requestId, name].concat(fields || []));
};

EqController.prototype.ReadParameter = function(values, position) {
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

EqController.prototype.HandleDefinitions = function(values) {
    var count = Number(values[5]);
    var position = 6;
    this.filterDefinitions = {};
    for (var index = 0; index < count; index++) {
        var filterId = Number(values[position++]);
        var type = String(values[position++]);
        var defaultBypass = Number(values[position++]) !== 0;
        var parameterCount = Number(values[position++]);
        var parameters = [];
        for (var parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++) {
            var parameter = this.ReadParameter(values, position);
            parameters.push(parameter.value);
            position = parameter.next;
        }
        this.filterDefinitions[filterId] = {
            id: filterId,
            type: type,
            defaultBypass: defaultBypass,
            parameters: parameters
        };
        var definition = this.filterDefinitions[filterId];
        var hasFrequency = this.FindParameter(definition, "freq")
            || this.FindParameter(definition, "pivot")
            || this.FindParameter(definition, "frequency");
        this.SendDialValueCount(
            filterId,
            this.FindParameter(definition, "q") ? 3 : (hasFrequency ? 2 : 1)
        );
        this.SendDisplayRanges(filterId, definition);
    }
    this.FlushPendingDialValues();
    this.ApplyLinkColor();
    this.FlushFilterLimits();
};

EqController.prototype.ParameterDisplay = function(parameter) {
    if (parameter.name === "gain") return { decimals: 1, suffix: " dB" };
    if (parameter.name === "freq" || parameter.name === "pivot" ||
        parameter.name === "frequency") return { decimals: 0, suffix: " Hz" };
    if (parameter.name === "q") return { decimals: 2, suffix: "" };
    return { decimals: 2, suffix: "" };
};

EqController.prototype.SendDisplayRanges = function(filterId, definition) {
    for (var index = 0; index < definition.parameters.length; index++) {
        var parameter = definition.parameters[index];
        var ring = this.ParameterRing(parameter);
        if (!ring) continue;
        var display = this.ParameterDisplay(parameter);
        outlet(1, [
            "script", "sendbox", this.DialVarName(filterId), "displayRange",
            ring, parameter.minimum, parameter.maximum,
            parameter.logarithmic ? 1 : 0, display.decimals, display.suffix
        ]);
    }
};

EqController.prototype.FlushPendingDialValues = function() {
    var pending = this.pendingDialValues;
    this.pendingDialValues = {};
    for (var key in pending) {
        if (!pending.hasOwnProperty(key)) continue;
        var parts = key.split(":");
        this.HandleDial(Number(parts[0]), Number(parts[1]), pending[key]);
    }
};

EqController.prototype.SendDialValueCount = function(filterId, valueCount) {
    outlet(1, ["script", "sendbox", this.DialVarName(filterId), "valueCount", valueCount]);
};

EqController.prototype.FindParameter = function(definition, name) {
    if (!definition) return null;
    for (var index = 0; index < definition.parameters.length; index++) {
        if (definition.parameters[index].name === name) return definition.parameters[index];
    }
    return null;
};

EqController.prototype.GetRingParameter = function(definition, ringIndex) {
    if (ringIndex === 1) return this.FindParameter(definition, "gain");
    if (ringIndex === 2) {
        return this.FindParameter(definition, "freq")
            || this.FindParameter(definition, "pivot")
            || this.FindParameter(definition, "frequency");
    }
    if (ringIndex === 3) return this.FindParameter(definition, "q");
    return null;
};

EqController.prototype.ParameterIndex = function(definition, parameter) {
    if (!definition || !parameter) return -1;
    for (var index = 0; index < definition.parameters.length; index++) {
        if (definition.parameters[index] === parameter) return index;
    }
    return -1;
};

EqController.prototype.SendParameterGesture = function(filterId, parameter, normalized) {
    var definition = this.filterDefinitions[filterId];
    var parameterIndex = this.ParameterIndex(definition, parameter);
    if (parameterIndex < 0) return;
    outlet(3, "eq_parameter_gesture",
        this.selectedBankId, filterId, parameterIndex,
        Math.max(0, Math.min(1, Number(normalized))));
};

EqController.prototype.QueueParameterUpdate = function(
    bankId,
    filterId,
    parameter,
    normalized
) {
    var definition = this.filterDefinitions[filterId];
    var parameterIndex = this.ParameterIndex(definition, parameter);
    if (parameterIndex < 0) return;
    var value = Math.max(0, Math.min(1, Number(normalized)));
    this.parameterDispatcher.Enqueue(
        [bankId, filterId, parameterIndex].join(":"),
        {
            bankId: Number(bankId),
            filterId: Number(filterId),
            parameterIndex: parameterIndex,
            parameter: parameter,
            normalized: value
        }
    );
};

EqController.prototype.FlushParameterUpdate = function(update) {
    outlet(3, "eq_parameter_gesture",
        update.bankId, update.filterId, update.parameterIndex, update.normalized);
    this.SendCommand("eq.set_parameter", [
        update.bankId,
        update.filterId,
        update.parameter.name,
        this.ToAbsolute(update.parameter, update.normalized)
    ]);
};

EqController.prototype.ToAbsolute = function(parameter, normalized) {
    var value = Math.max(0, Math.min(1, Number(normalized)));
    if (parameter.logarithmic && parameter.minimum > 0) {
        return parameter.minimum
            * Math.pow(parameter.maximum / parameter.minimum, value);
    }
    return parameter.minimum + value * (parameter.maximum - parameter.minimum);
};

EqController.prototype.ToNormalized = function(parameter, absolute) {
    if (!parameter) return 0;
    var value = Number(absolute);
    if (parameter.logarithmic && parameter.minimum > 0) {
        return Math.max(0, Math.min(1,
            Math.log(value / parameter.minimum)
            / Math.log(parameter.maximum / parameter.minimum)
        ));
    }
    if (parameter.maximum === parameter.minimum) return 0;
    return Math.max(0, Math.min(1,
        (value - parameter.minimum) / (parameter.maximum - parameter.minimum)
    ));
};

EqController.prototype.DialVarName = function(filterId) {
    return "eq.filter." + filterId + ".dial";
};

EqController.prototype.ControlVarName = function(filterId) {
    return "eq.filter." + filterId + ".control";
};

EqController.prototype.SendDialValue = function(filterId, attribute, value) {
    outlet(1, ["script", "sendbox", this.DialVarName(filterId), attribute, value]);
};

EqController.prototype.SendControlValue = function(filterId, value) {
    outlet(1, ["script", "sendbox", this.ControlVarName(filterId), "selection", value ? 1 : 0]);
};

EqController.prototype.ParameterRing = function(parameter) {
    if (!parameter) return 0;
    if (parameter.name === "gain") return 1;
    if (parameter.name === "freq" || parameter.name === "pivot" ||
        parameter.name === "frequency") return 2;
    if (parameter.name === "q") return 3;
    return 0;
};

EqController.prototype.HandleLinkColor = function(linkId, red, green, blue, alpha) {
    this.linkColor = String(linkId) === "-"
        ? null
        : [Number(red), Number(green), Number(blue), Number(alpha)];
    this.ApplyLinkColor();
};

EqController.prototype.ApplyLinkColor = function() {
    for (var filterId in this.filterDefinitions) {
        if (!this.filterDefinitions.hasOwnProperty(filterId)) continue;
        var definition = this.filterDefinitions[filterId];
        var coloredRings = {};
        for (var index = 0; index < definition.parameters.length; index++) {
            var ring = this.ParameterRing(definition.parameters[index]);
            if (!ring || coloredRings[ring]) continue;
            coloredRings[ring] = true;
            if (this.linkColor) {
                outlet(1, ["script", "sendbox", this.DialVarName(filterId),
                    "ringColor", ring].concat(this.linkColor));
            } else {
                outlet(1, ["script", "sendbox", this.DialVarName(filterId),
                    "clearRingColor", ring]);
            }
        }
    }
};

EqController.prototype.HandleFilterLimits = function(
    filterId,
    parameterIndex,
    minimum,
    maximum
) {
    var key = filterId + ":" + parameterIndex;
    this.pendingFilterLimits[key] = {
        filterId: Number(filterId),
        parameterIndex: Number(parameterIndex),
        minimum: Number(minimum),
        maximum: Number(maximum)
    };
    this.ApplyFilterLimits(this.pendingFilterLimits[key]);
};

EqController.prototype.HandleEqPreview = function(
    bankId,
    filterId,
    parameterIndex,
    absoluteValue
) {
    if (Number(bankId) !== this.selectedBankId) return;
    var definition = this.filterDefinitions[Number(filterId)];
    var parameter = definition && definition.parameters[Number(parameterIndex)];
    if (!parameter) return;
    var ring = this.ParameterRing(parameter);
    if (!ring) return;
    var attributes = ["primaryValue", "secondaryValue", "tertiaryValue"];
    this.SendDialValue(Number(filterId), attributes[ring - 1],
        this.ToNormalized(parameter, Number(absoluteValue)));
};

EqController.prototype.ApplyFilterLimits = function(limit) {
    var definition = this.filterDefinitions[limit.filterId];
    var parameter = definition && definition.parameters[limit.parameterIndex];
    var ring = this.ParameterRing(parameter);
    if (!parameter || !ring) return;
    outlet(1, ["script", "sendbox", this.DialVarName(limit.filterId),
        "limits", ring, this.ToNormalized(parameter, limit.minimum),
        this.ToNormalized(parameter, limit.maximum)]);
};

EqController.prototype.FlushFilterLimits = function() {
    for (var key in this.pendingFilterLimits) {
        if (this.pendingFilterLimits.hasOwnProperty(key)) {
            this.ApplyFilterLimits(this.pendingFilterLimits[key]);
        }
    }
};

EqController.prototype.HandleDial = function(filterId, ringIndex, normalized) {
    var definition = this.filterDefinitions[filterId];
    var parameter = this.GetRingParameter(definition, ringIndex);
    if (!parameter) {
        this.pendingDialValues[filterId + ":" + ringIndex] = Number(normalized);
        return;
    }
    this.QueueParameterUpdate(
        this.selectedBankId, filterId, parameter, normalized);
};

EqController.prototype.HandleControl = function(filterId, buttonIndex, value) {
    if (Number(buttonIndex) === 1) {
        this.SendFilterBypass(filterId, value);
    } else if (Number(buttonIndex) === 2) {
        this.SendFilterReset(filterId);
    }
};

EqController.prototype.SendFilterBypass = function(filterId, value) {
    var bypass = Number(value) ? 1 : 0;
    outlet(3, "eq_bypass_gesture",
        this.selectedBankId, filterId, bypass);
    this.SendCommand("eq.set_bypass", [
        this.selectedBankId, filterId, bypass
    ]);
};

EqController.prototype.SendFilterReset = function(filterId) {
    outlet(3, "eq_filter_reset_gesture",
        this.selectedBankId, filterId);
    this.SendCommand("eq.reset_filter", [this.selectedBankId, filterId]);
};

EqController.prototype.HandleGlobal = function(buttonIndex, value) {
    var action = Number(buttonIndex);
    var isPressed = Number(value) ? 1 : 0;
    if (action === 1) {
        this.SendCommand("eq.set_chain_bypass", [isPressed]);
    } else if (action === 2) {
        this.SendCommand("eq.set_chain_solo", [isPressed]);
    } else if (action === 3 && isPressed) {
        outlet(3, "eq_bank_reset_gesture", this.selectedBankId);
        this.SendCommand("eq.reset", [this.selectedBankId]);
    }
};

EqController.prototype.HandleLocal = function(values) {
    if (values.length < 2) return;
    var filterId = Number(values[0]);
    var action = String(values[1]);
    if (action === "reset") {
        this.SendFilterReset(filterId);
        return;
    }
    if (action === "bypass") {
        this.SendFilterBypass(filterId, values[2]);
        return;
    }
    var parameter = this.GetRingParameter(
        this.filterDefinitions[filterId],
        Number(action)
    );
    if (!parameter) parameter = this.FindParameter(this.filterDefinitions[filterId], action);
    if (!parameter) return;
    this.QueueParameterUpdate(
        this.selectedBankId, filterId, parameter, values[2]);
};

EqController.prototype.HandleEqSnapshot = function(values) {
    this.selectedBankId = Number(values[5]);
    var position = 9;
    var bankCount = Number(values[8]);
    for (var bankIndex = 0; bankIndex < bankCount; bankIndex++) {
        var bankId = Number(values[position++]);
        position++;
        var filterCount = Number(values[position++]);
        for (var filterIndex = 0; filterIndex < filterCount; filterIndex++) {
            var filterId = Number(values[position++]);
            var bypass = Number(values[position++]);
            var valueCount = Number(values[position++]);
            var definition = this.filterDefinitions[filterId];
            if (bankId === this.selectedBankId && definition) {
                var dialAttributes = ["primaryValue", "secondaryValue", "tertiaryValue"];
                for (var valueIndex = 0; valueIndex < valueCount; valueIndex++) {
                    var parameter = definition.parameters[valueIndex];
                    var ringIndex = parameter.name === "gain" ? 1
                        : (parameter.name === "freq" || parameter.name === "pivot"
                            || parameter.name === "frequency" ? 2 : 3);
                    this.SendDialValue(
                        filterId,
                        dialAttributes[ringIndex - 1],
                        this.ToNormalized(parameter, values[position + valueIndex])
                    );
                }
                this.SendControlValue(filterId, bypass);
            }
            position += valueCount;
        }
    }
};

EqController.prototype.HandleSnapshot = function(values) {
    if (values.length < 6 || String(values[0]) !== "snapshot") return;
    var store = String(values[3]);
    if (store === "definitions") this.HandleDefinitions(values);
    else if (store === "eq") this.HandleEqSnapshot(values);
};

var controller = new EqController();

function inletassist(index) {
    assist([
        "Local: filter controls; link_color, filter_limits, eq_preview <bankId> <filterId> <parameterIndex> <absoluteValue>; processor_limits is ignored",
        "Host snapshot: definitions and EQ state"
    ][index] || "");
}

function outletassist(index) {
    assist([
        "Host commands: eq.set_parameter, eq.set_bypass, eq.reset_filter, eq.set_chain_bypass, eq.set_chain_solo, eq.reset",
        "thispatcher: script sendbox eq.filter.<id>.dial|control <attribute> <value>; previews apply directly to the selected dial",
        "Diagnostics: error <code>",
        "Link gestures: eq_parameter_gesture, eq_bypass_gesture, eq_filter_reset_gesture, eq_bank_reset_gesture"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) controller.HandleLocal(values);
    else if (inlet === 1 && values.length && String(values[0]) === "snapshot") {
        controller.HandleSnapshot(values);
    }
}

function dial() {
    if (inlet !== 0) return;
    var values = arrayfromargs(arguments);
    if (values.length === 2) {
        controller.HandleDial(Number(values[0]), 1, Number(values[1]));
        return;
    }
    controller.HandleDial(Number(values[0]), Number(values[1]), Number(values[2]));
}

function filter() {
    if (inlet !== 0) return;
    controller.HandleLocal(arrayfromargs(arguments));
}

function control() {
    if (inlet !== 0) return;
    var values = arrayfromargs(arguments);
    controller.HandleControl(Number(values[0]), Number(values[1]), Number(values[2]));
}

function eqglobal() {
    if (inlet !== 0) return;
    var values = arrayfromargs(arguments);
    controller.HandleGlobal(Number(values[0]), Number(values[1]));
}

function snapshot() {
    if (inlet === 1) controller.HandleSnapshot(["snapshot"].concat(arrayfromargs(arguments)));
}

function link_color(linkId, red, green, blue, alpha) {
    if (inlet === 0) controller.HandleLinkColor(
        String(linkId), Number(red), Number(green), Number(blue), Number(alpha));
}

function filter_limits(filterId, parameterIndex, minimum, maximum) {
    if (inlet === 0) controller.HandleFilterLimits(
        Number(filterId), Number(parameterIndex), Number(minimum), Number(maximum));
}

function eq_preview(bankId, filterId, parameterIndex, absoluteValue) {
    if (inlet === 0) controller.HandleEqPreview(
        Number(bankId), Number(filterId), Number(parameterIndex),
        Number(absoluteValue));
}

function processor_limits() {}
function event() {}
