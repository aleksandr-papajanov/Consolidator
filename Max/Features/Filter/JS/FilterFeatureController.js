autowatch = 1;
inlets = 2;
outlets = 2;

include("../../Shared/JS/DictionaryReader.js");
include("../../Shared/JS/Messages/MessageEnvelope.js");
include("../../Shared/JS/Messages/MessageFactory.js");
include("FeatureMessageAdapter.js");

function FilterFeatureController(slot) {
    this.slot = Number(slot);
    this.adapter = new FeatureMessageAdapter("filter", MessageFactory, {});
    this.configuration = null;
    this.configurationName = "";
    this.selectedFilter = null;
    this.filterType = "";
    this.controls = {
        gain: { varName: "gain_dial" },
        frequency: { varName: "freq.numbox" },
        q: { varName: "q.numbox" },
        bypass: { varName: "bypass" },
        reset: { varName: "reset" }
    };
    this.parameters = [];
    this.bypassed = false;
    this.instanceRecovered = null;
    this.nativeReady = false;
}

FilterFeatureController.prototype.Emit = function(type, payload) {
    payload.filterId = this.slot;
    var envelope = MessageEnvelope.create(type, "filter", payload, "filter");
    this.adapter.EmitEnvelope(envelope, function(message) {
        var dictionary = MessageFactory.toMax(message);
        outlet(0, "message", dictionary.name);
    });
};

FilterFeatureController.prototype.Number = function(value, fallback) {
    var number = Number(value);
    return isFinite(number) ? number : fallback;
};

FilterFeatureController.prototype.Array = function(value) {
    return value instanceof Array ? value : [];
};

FilterFeatureController.prototype.Configure = function(dictionaryName) {
    try {
        this.nativeReady = false;
        this.configurationName = String(dictionaryName);
        this.configuration = new DictionaryReader(this.configurationName);
        var selected = this.Number(this.configuration.selected, this.slot);
        this.selectedFilter = this.configuration.filters[selected];
        if (!this.selectedFilter) throw new Error("missing_filter_configuration");
        this.filterType = String(this.selectedFilter.type || "");

        this.parameters = [];
        var parameterNames = this.ParameterNames();
        for (var i = 0; i < parameterNames.length; i++) {
            var name = parameterNames[i];
            var parameter = this.selectedFilter.parameters[name];
            var definition = {
                name: name,
                control: String(parameter.control || ""),
                scale: String(parameter.scale || "linear"),
                min: this.Number(parameter.min, 0),
                max: this.Number(parameter.max, 1),
                defaultValue: this.Number(parameter.default, 0)
            };
            this.parameters.push(definition);
        }

        this.BuildControls();
        this.ApplyLayout();
        this.Emit("filter.define", { contractName: this.configurationName });
        this.ApplyPendingInstanceState();
    }
    catch (error) {
        post("FilterFeatureController: invalid_filter_configuration_dictionary\n");
    }
};

FilterFeatureController.prototype.ParameterNames = function() {
    var names = [];
    var parameters = this.selectedFilter.parameters || {};
    for (var key in parameters) {
        if (Object.prototype.hasOwnProperty.call(parameters, key)) {
            names.push(String(key));
        }
    }

    if (names.length > 0) return names;
    if (this.filterType === "gain") return ["gain"];
    if (this.filterType === "tilt") return ["gain", "pivot"];
    return ["gain", "freq", "q"];
};

FilterFeatureController.prototype.BuildControls = function() {
    var ids = ["gain", "frequency", "q", "bypass", "reset"];

    for (var i = 0; i < ids.length; i++) {
        var id = ids[i];
        var control = this.controls[id];
        var controlConfiguration = this.configuration.controls[id] || {};
        var position = this.Array(controlConfiguration.defaultPosition);
        if (position.length !== 4) continue;

        var active = false;
        for (var p = 0; p < this.parameters.length; p++) {
            if (this.parameters[p].control === id) active = true;
        }

        var alwaysEnabled = this.Number(
            controlConfiguration.defaultEnabled, 0) === 1;
        var visible = true;
        var enabled = active || alwaysEnabled;
        var layout = (this.configuration.layouts[this.filterType] || {})[id] || {};
        var visibleValue = layout.visible;
        var enabledValue = layout.enabled;
        if (visibleValue !== null && visibleValue !== undefined) {
            visible = this.Number(visibleValue, 1) !== 0;
        }
        if (enabledValue !== null && enabledValue !== undefined) {
            enabled = this.Number(enabledValue, enabled ? 1 : 0) !== 0;
        }

        control.position = position;
        control.active = active;
        control.alwaysEnabled = alwaysEnabled;
        control.visible = visible;
        control.enabled = enabled;
        control.outputValue = this.Number(controlConfiguration.outputValue, 0) === 1;
    }
};

FilterFeatureController.prototype.ApplyLayout = function() {
    var ids = ["gain", "frequency", "q", "bypass", "reset"];
    var color = this.ParseColor(this.selectedFilter.color);

    for (var i = 0; i < ids.length; i++) {
        var id = ids[i];
        var state = this.controls[id];
        if (!state) continue;
        this.SendControl(id, "move", state.position);
        this.SendControl(id, state.visible ? "show" : "hide");
        this.SendControl(id, "color", color);
    }

    this.ApplyEnabledState();
};

FilterFeatureController.prototype.ApplyEnabledState = function() {
    var ids = ["gain", "frequency", "q", "bypass", "reset"];
    for (var i = 0; i < ids.length; i++) {
        var id = ids[i];
        var state = this.controls[id];
        if (!state) continue;
        var enabled = state.alwaysEnabled || (!this.bypassed && state.enabled);
        this.SendControl(id, enabled ? "enable" : "disable");
    }
};

FilterFeatureController.prototype.ApplyValues = function(values) {
    if (!this.controls || values.length < this.parameters.length + 1) return;

    for (var i = 0; i < this.parameters.length; i++) {
        var control = this.parameters[i].control;
        if (control) this.SendControl(control, "set", [values[i]]);
    }

    this.bypassed = this.Number(values[this.parameters.length], 0) === 1;
    this.ApplyEnabledState();
};

FilterFeatureController.prototype.ApplyDefaults = function() {
    var values = [];
    for (var i = 0; i < this.parameters.length; i++) {
        var parameter = this.parameters[i];
        var normalized = parameter.scale === "logarithmic"
            ? Math.log(parameter.defaultValue / parameter.min) /
              Math.log(parameter.max / parameter.min)
            : (parameter.defaultValue - parameter.min) /
              (parameter.max - parameter.min);
        values.push(Math.max(0, Math.min(1, normalized)));
    }
    values.push(0);
    this.ApplyValues(values);
};

FilterFeatureController.prototype.OutputActiveValues = function() {
    var ids = ["gain", "frequency", "q", "bypass", "reset"];
    for (var i = 0; i < ids.length; i++) {
        var state = this.controls[ids[i]];
        if (state && (state.active || state.outputValue)) {
            this.SendControl(ids[i], "outputvalue");
        }
    }
};

FilterFeatureController.prototype.ApplyPendingInstanceState = function() {
    if (this.instanceRecovered === null || !this.nativeReady) return;

    if (this.instanceRecovered) this.OutputActiveValues();
    else {
        this.ApplyDefaults();
        this.Emit("filter.reset", {});
    }
    this.instanceRecovered = null;
};

FilterFeatureController.prototype.SendControl = function(controlId, action, values) {
    var control = this.controls[controlId];
    if (!control) return;

    var command = ["script", "sendbox", control.varName];
    var commandValues = values || [];
    if (action === "move") command = command.concat(["presentation_rect"], commandValues);
    else if (action === "show") command = command.concat(["hidden", 0]);
    else if (action === "hide") command = command.concat(["hidden", 1]);
    else if (action === "enable") command = command.concat(["active", 1]);
    else if (action === "disable") command = command.concat(["active", 0]);
    else if (action === "set") command = command.concat(["set", commandValues[0]]);
    else if (action === "outputvalue") command = command.concat(["outputvalue"]);
    else if (action === "color") {
        var attribute = control.varName.indexOf(".numbox") !== -1
            ? "activeslidercolor"
            : (controlId === "bypass" || controlId === "reset"
                ? "lcdcolor"
                : "activedialcolor");
        command = command.concat([attribute], commandValues);
    }
    else return;

    outlet(1, command);
};

FilterFeatureController.prototype.ParseColor = function(value) {
    var text = String(value || "#FFFFFF");
    if (text.charAt(0) === "#") text = text.slice(1);
    if (text.length !== 6 && text.length !== 8) return [1, 1, 1, 1];

    function component(offset) {
        return parseInt(text.substr(offset, 2), 16) / 255;
    }

    return [
        component(0),
        component(2),
        component(4),
        text.length === 8 ? component(6) : 1
    ];
};

FilterFeatureController.prototype.HandleLocalDictionary = function(name) {
    this.Configure(name);
};

FilterFeatureController.prototype.HandleLocalCommand = function(command, values) {
    if (command === "update" && values.length === 2) {
        if (!this.configuration || !this.nativeReady) return;
        this.Emit("filter.control.update", {
            control: String(values[0]),
            value: Number(values[1])
        });
    }
    else if (command === "instance_state" && values.length === 1) {
        this.instanceRecovered = Number(values[0]) === 1;
        this.ApplyPendingInstanceState();
    }
    else if (command === "reset" && values.length === 0) {
        this.ApplyDefaults();
        this.Emit("filter.reset", {});
    }
};

FilterFeatureController.prototype.HandleNativeStatus = function(state, values) {
    if (state === "ready") {
        this.nativeReady = true;
        this.ApplyPendingInstanceState();
    }
    else if (state === "values") {
        this.ApplyValues(values);
    }
    this.adapter.HandleStatus(state, values);
};

var controller = new FilterFeatureController(jsarguments[1]);

function dictionary(name) {
    if (inlet === 0) controller.HandleLocalDictionary(name);
}

function update(control, value) {
    if (inlet === 0) controller.HandleLocalCommand("update", [control, value]);
}

function instance_state(recovered) {
    if (inlet === 0) controller.HandleLocalCommand("instance_state", [recovered]);
}

function reset() {
    if (inlet === 0) controller.HandleLocalCommand("reset", []);
}

function status(state) {
    if (inlet === 1) {
        controller.HandleNativeStatus(state, arrayfromargs(arguments).slice(1));
    }
}

function message() {
    if (inlet === 1) {
        controller.HandleNativeStatus(messagename, arrayfromargs(arguments));
    }
}

function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) controller.HandleLocalCommand(messagename, values);
    else if (messagename === "status") {
        controller.HandleNativeStatus(values[0], values.slice(1));
    }
}
