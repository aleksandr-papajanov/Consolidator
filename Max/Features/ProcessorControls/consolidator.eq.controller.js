autowatch = 1;
inlets = 2;
outlets = 3;

function EqController() {
    this.requestId = 0;
    this.selectedBankId = 1;
    this.filterDefinitions = {};
    this.pendingDialValues = {};
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
    }
    this.FlushPendingDialValues();
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

EqController.prototype.HandleDial = function(filterId, ringIndex, normalized) {
    var definition = this.filterDefinitions[filterId];
    var parameter = this.GetRingParameter(definition, ringIndex);
    if (!parameter) {
        this.pendingDialValues[filterId + ":" + ringIndex] = Number(normalized);
        return;
    }
    this.SendCommand("eq.set_parameter", [
        this.selectedBankId,
        filterId,
        parameter.name,
        this.ToAbsolute(parameter, normalized)
    ]);
};

EqController.prototype.HandleControl = function(filterId, buttonIndex, value) {
    if (Number(buttonIndex) === 1) {
        this.SendCommand("eq.set_bypass", [
            this.selectedBankId,
            filterId,
            Number(value) ? 1 : 0
        ]);
    } else if (Number(buttonIndex) === 2) {
        this.SendCommand("eq.reset_filter", [this.selectedBankId, filterId]);
    }
};

EqController.prototype.HandleGlobal = function(buttonIndex, value) {
    var action = Number(buttonIndex);
    var isPressed = Number(value) ? 1 : 0;
    if (action === 1) {
        this.SendCommand("eq.set_chain_bypass", [this.selectedBankId, isPressed]);
    } else if (action === 2 && isPressed) {
        this.SendCommand("eq.reset", [this.selectedBankId]);
    }
};

EqController.prototype.HandleLocal = function(values) {
    if (values.length < 2) return;
    var filterId = Number(values[0]);
    var action = String(values[1]);
    if (action === "reset") {
        this.SendCommand("eq.reset_filter", [this.selectedBankId, filterId]);
        return;
    }
    if (action === "bypass") {
        this.SendCommand("eq.set_bypass", [
            this.selectedBankId,
            filterId,
            Number(values[2]) ? 1 : 0
        ]);
        return;
    }
    var parameter = this.GetRingParameter(
        this.filterDefinitions[filterId],
        Number(action)
    );
    if (!parameter) parameter = this.FindParameter(this.filterDefinitions[filterId], action);
    if (!parameter) return;
    this.SendCommand("eq.set_parameter", [
        this.selectedBankId,
        filterId,
        parameter.name,
        this.ToAbsolute(parameter, values[2])
    ]);
};

EqController.prototype.HandleEqSnapshot = function(values) {
    this.selectedBankId = Number(values[5]);
    var position = 7;
    var bankCount = Number(values[6]);
    for (var bankIndex = 0; bankIndex < bankCount; bankIndex++) {
        var bankId = Number(values[position++]);
        position++;
        position++;
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
        "Local: <filterId> <gain|frequency|q|reset|bypass> <normalized-value>",
        "Host snapshot: definitions and EQ state"
    ][index] || "");
}

function outletassist(index) {
    assist([
        "Host commands: eq.set_parameter, eq.set_bypass, eq.reset_filter, eq.set_chain_bypass, eq.reset",
        "thispatcher: script sendbox eq.filter.<id>.dial|control <attribute> <value>",
        "Diagnostics: error <code>"
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

function event() {}
