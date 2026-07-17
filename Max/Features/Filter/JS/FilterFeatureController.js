autowatch = 1;
inlets = 2;
outlets = 2;

include("../../Shared/JS/Messages/MessageEnvelope.js");
include("../../Shared/JS/Messages/MessageFactory.js");
include("FeatureMessageAdapter.js");

function ControlRegistry(controlMap) {
    this.controlMap = controlMap;
}

ControlRegistry.prototype.CreateScriptCommand = function(controlId, action, values) {
    var varname = this.controlMap[controlId];
    if (!varname) {
        return null;
    }

    var command = ["script", "sendbox", varname];
    if (action === "move") return command.concat(["presentation_rect"], values);
    if (action === "show") return command.concat(["hidden", 0]);
    if (action === "hide") return command.concat(["hidden", 1]);
    if (action === "enable") return command.concat(["active", 1]);
    if (action === "disable") return command.concat(["active", 0]);
    if (action === "set") return command.concat(["set", values[0]]);
    if (action === "outputvalue") return command.concat(["outputvalue"]);
    if (action === "color") {
        var colorAttribute = varname.indexOf(".numbox") !== -1
            ? "activeslidercolor"
            : (varname === "bypass" || varname === "reset")
                ? "lcdcolor"
                : "activedialcolor";
        return command.concat([colorAttribute], values);
    }
    return null;
};

function FilterFeatureController(slot) {
    this.slot = Number(slot);
    this.controls = new ControlRegistry({
        gain: "gain_dial",
        frequency: "freq.numbox",
        q: "q.numbox",
        bypass: "bypass",
        reset: "reset"
    });
    this.adapter = new FeatureMessageAdapter("filter", MessageFactory, {});
}

FilterFeatureController.prototype.Emit = function(type, payload) {
    payload.filterId = this.slot;
    var envelope = MessageEnvelope.create(type, "filter", payload, "filter");
    this.adapter.EmitEnvelope(envelope, function(message) {
        var dictionary = MessageFactory.toMax(message);
        outlet(0, "message", dictionary.name);
    });
};

FilterFeatureController.prototype.HandleLocalDictionary = function(name) {
    var configuration;
    try {
        configuration = new Dict(String(name));
    }
    catch (error) {
        post("FilterFeatureController: invalid_filter_configuration_dictionary\n");
        return;
    }
    this.Emit("filter.define", { contractName: configuration.name });
};

FilterFeatureController.prototype.HandleLocalCommand = function(command, values) {
    if (command === "update" && values.length === 2) {
        this.Emit("filter.control.update", { control: String(values[0]), value: Number(values[1]) });
    }
    else if (command === "instance_state" && values.length === 1) {
        this.Emit("filter.instance.state", { recovered: Number(values[0]) });
    }
    else if (command === "reset" && values.length === 0) {
        this.Emit("filter.reset", {});
    }
};

FilterFeatureController.prototype.HandleNativeEnvelope = function(dictionaryName) {
    var envelope = MessageFactory.fromMax(dictionaryName);
    if (!envelope || envelope.type !== "filter.control") {
        return;
    }

    var command = this.controls.CreateScriptCommand(
        envelope.payloadValue("control"),
        envelope.payloadValue("action"),
        envelope.payloadValue("values") || []
    );
    if (command) {
        outlet(1, command);
    }
};

FilterFeatureController.prototype.HandleNativeStatus = function(state, values) {
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
    if (inlet === 1) controller.HandleNativeStatus(state, arrayfromargs(arguments).slice(1));
}

function message() {
    var values = arrayfromargs(arguments);
    if (inlet === 1) controller.HandleNativeEnvelope(values);
}

function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) controller.HandleLocalCommand(messagename, values);
    else if (messagename === "status") controller.HandleNativeStatus(values[0], values.slice(1));
}
