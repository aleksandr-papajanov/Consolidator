autowatch = 1;
inlets = 2;
outlets = 2;

include("../Shared/JS/DictionaryReader.js");

function FilterFeatureController(slot) {
    this.slot = Number(slot);
    this.configurationDictionary = null;
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
    this.filterReady = false;
}

FilterFeatureController.ConfigurationPath = "Config/ConsolidatorSettings.json";

FilterFeatureController.prototype.LoadConfiguration = function() {
    try {
        this.configurationDictionary = new Dict();
        this.configurationDictionary.import_json(FilterFeatureController.ConfigurationPath);
        this.Configure(this.configurationDictionary.name);
    }
    catch (error) {
        post("FilterFeatureController: cannot_read_filter_configuration\n");
    }
};

FilterFeatureController.prototype.SendFilterCommand = function() {
    var values = arrayfromargs(arguments);
    if (values.length === 1) {
        outlet(0, values[0]);
    }
    else if (values.length === 2) {
        outlet(0, values[0], values[1]);
    }
    else if (values.length === 3) {
        outlet(0, values[0], values[1], values[2]);
    }
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
        this.filterReady = false;
        this.configurationName = String(dictionaryName);
        this.configuration = new DictionaryReader(this.configurationName);
        this.selectedFilter = this.configuration.filters[this.slot];
        if (!this.selectedFilter) throw new Error("missing_filter_configuration");
        this.parameters = [];
        this.SendFilterCommand("configure");
    }
    catch (error) {
        post("FilterFeatureController: invalid_filter_configuration_dictionary\n");
    }
};

FilterFeatureController.prototype.ControlForParameter = function(name) {
    if (name === "freq" || name === "pivot") return "frequency";
    return name;
};

FilterFeatureController.prototype.ApplyDefinition = function(values) {
    if (!this.configuration || values.length < 3) return;
    this.filterType = String(values[0]);
    this.bypassed = Number(values[1]) !== 0;
    var parameterCount = Number(values[2]);
    if (!isFinite(parameterCount) || parameterCount < 1 || values.length !== 3 + parameterCount * 5) return;
    this.parameters = [];
    var position = 3;
    for (var index = 0; index < parameterCount; index++) {
        var name = String(values[position++]);
        this.parameters.push({
            name: name,
            control: this.ControlForParameter(name),
            min: this.Number(values[position++], 0),
            max: this.Number(values[position++], 1),
            scale: String(values[position++]),
            defaultValue: this.Number(values[position++], 0)
        });
    }
    this.BuildControls();
    this.ApplyLayout();
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
        var enabled = this.filterReady &&
            (state.alwaysEnabled || (!this.bypassed && state.enabled));
        this.SendControl(id, enabled ? "enable" : "disable");
    }
};

FilterFeatureController.prototype.ApplyValues = function(values) {
    if (!this.controls || values.length !== this.parameters.length + 1) return;

    for (var i = 0; i < this.parameters.length; i++) {
        var control = this.parameters[i].control;
        if (control) this.SendControl(control, "set", [this.ToNormalized(this.parameters[i], values[i])]);
    }

    this.bypassed = this.Number(values[this.parameters.length], 0) === 1;
    this.ApplyEnabledState();
};

FilterFeatureController.prototype.ParameterForControl = function(control) {
    for (var i = 0; i < this.parameters.length; i++) {
        if (this.parameters[i].control === control) return this.parameters[i];
    }
    return null;
};

FilterFeatureController.prototype.ToAbsolute = function(definition, value) {
    var normalized = Math.max(0, Math.min(1, this.Number(value, 0)));
    if (definition.scale === "logarithmic" && definition.min > 0 && definition.max > 0) {
        return definition.min * Math.pow(definition.max / definition.min, normalized);
    }
    return definition.min + (definition.max - definition.min) * normalized;
};

FilterFeatureController.prototype.ToNormalized = function(definition, value) {
    var absolute = this.Number(value, definition.defaultValue);
    if (definition.scale === "logarithmic" && definition.min > 0 && definition.max > 0) {
        absolute = Math.max(definition.min, Math.min(definition.max, absolute));
        return Math.log(absolute / definition.min) / Math.log(definition.max / definition.min);
    }
    if (definition.max === definition.min) return 0;
    return Math.max(0, Math.min(1, (absolute - definition.min) / (definition.max - definition.min)));
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

FilterFeatureController.prototype.HandleLocalCommand = function(command, values) {
    if (command === "update" && values.length === 2) {
        if (!this.configuration || !this.filterReady) return;
        if (String(values[0]) === "bypass") {
            this.SendFilterCommand("update", "bypass", Number(values[1]));
            return;
        }
        var definition = this.ParameterForControl(String(values[0]));
        if (!definition) return;
        this.SendFilterCommand("update", definition.name, this.ToAbsolute(definition, values[1]));
    }
    else if (command === "reset" && values.length === 0) {
        this.SendFilterCommand("reset");
    }
};

FilterFeatureController.prototype.HandleFilterStatus = function(state, values) {
    if (state === "definition") {
        this.ApplyDefinition(values);
    }
    else if (state === "ready") {
        this.filterReady = true;
        this.ApplyEnabledState();
    }
    else if (state === "values") {
        this.ApplyValues(values);
    }
};

var controller = new FilterFeatureController(jsarguments[1]);

function inletassist(index) {
    var descriptions = [
        "Local UI commands: update <control> <0..1>, reset",
        "Filter status: definition, ready, and absolute values"
    ];
    assist(descriptions[index] || "");
}

function outletassist(index) {
    var descriptions = [
        "Local consolidator.filter.js commands: configure, update <parameter> <absolute>, reset",
        "thispatcher commands for filter controls"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function loadbang() { controller.LoadConfiguration(); }

function update(control, value) {
    if (inlet === 0) controller.HandleLocalCommand("update", [control, value]);
}

function reset() {
    if (inlet === 0) controller.HandleLocalCommand("reset", []);
}

function status(state) {
    if (inlet === 1) {
        controller.HandleFilterStatus(state, arrayfromargs(arguments).slice(1));
    }
}

function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) controller.HandleLocalCommand(messagename, values);
    else if (messagename === "status") {
        controller.HandleFilterStatus(values[0], values.slice(1));
    }
}
