autowatch = 1;
inlets = 2;
outlets = 3;

// Inlet 0: local commands source <one-based-index>, channel <one-based-index>.
// Inlet 1: live.routing updates source_value, channel_value, source_options, channel_options.
// Outlet 0: live.routing commands source|channel <zero-based-index>.
// Outlet 1: routing.control updates.
// Outlet 2: diagnostics error <code>.

function RoutingController() {
    this.source = { items: [], value: null };
    this.channel = { items: [], value: null };
}

RoutingController.prototype.HandleSelection = function(name, value) {
    var state = this.State(name);
    var index = Math.floor(Number(value));
    if (!state || index < 1 || index > state.items.length) return;
    outlet(0, name, index - 1);
};

RoutingController.prototype.HandleRoutingUpdate = function(command, values) {
    var separator = command.indexOf("_");
    if (separator < 1) {
        this.Error("invalid_routing_update");
        return;
    }

    var name = command.substring(0, separator);
    var update = command.substring(separator + 1);
    var state = this.State(name);
    if (!state) {
        this.Error("invalid_routing_target");
        return;
    }

    if (update === "options") {
        state.items = this.NormalizeItems(values);
        this.SendItems(name, state);
        this.SendSelection(name, state);
        return;
    }
    if (update === "value") {
        state.value = values.length > 0 ? values[0] : null;
        this.SendSelection(name, state);
        return;
    }
    this.Error("unsupported_routing_update");
};

RoutingController.prototype.State = function(name) {
    if (name === "source") return this.source;
    if (name === "channel") return this.channel;
    return null;
};

RoutingController.prototype.NormalizeItems = function(values) {
    var items = values.slice();
    if (items.length > 0 && String(items[0]) === "list") items.shift();
    if (items.length === 0 || String(items[0]) === "<none>") return [];
    var normalized = [];
    for (var index = 0; index < items.length; index++) {
        var item = String(items[index]);
        if (item) normalized.push(item);
    }
    return normalized;
};

RoutingController.prototype.FindSelection = function(state) {
    if (state.items.length === 0) return 0;
    var numeric = Number(state.value);
    if (isFinite(numeric)) {
        var numericIndex = Math.floor(numeric) + 1;
        if (numericIndex >= 1 && numericIndex <= state.items.length) return numericIndex;
    }
    var current = String(state.value);
    for (var index = 0; index < state.items.length; index++) {
        if (state.items[index] === current) return index + 1;
    }
    return 1;
};

RoutingController.prototype.SendItems = function(name, state) {
    outlet(1, [name + "_items"].concat(state.items));
    outlet(1, name + "_enabled", state.items.length > 0 ? 1 : 0);
};

RoutingController.prototype.SendSelection = function(name, state) {
    outlet(1, name + "_selection", this.FindSelection(state));
};

RoutingController.prototype.Error = function(code) {
    outlet(2, "error", code);
};

var controller = new RoutingController();

function inletassist(index) {
    assist(index === 0
        ? "Local commands: source <one-based-index>, channel <one-based-index>"
        : "live.routing updates: source_value, channel_value, source_options, channel_options");
}

function outletassist(index) {
    assist([
        "live.routing commands: source|channel <zero-based-index>",
        "routing.control updates",
        "Diagnostics: error <code>"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function source(value) {
    if (inlet === 0) controller.HandleSelection("source", value);
}

function channel(value) {
    if (inlet === 0) controller.HandleSelection("channel", value);
}

function source_value() {
    if (inlet === 1) controller.HandleRoutingUpdate("source_value", arrayfromargs(arguments));
}

function channel_value() {
    if (inlet === 1) controller.HandleRoutingUpdate("channel_value", arrayfromargs(arguments));
}

function source_options() {
    if (inlet === 1) controller.HandleRoutingUpdate("source_options", arrayfromargs(arguments));
}

function channel_options() {
    if (inlet === 1) controller.HandleRoutingUpdate("channel_options", arrayfromargs(arguments));
}

function anything() {
    var values = arrayfromargs(arguments);
    if (inlet === 0) controller.HandleSelection(messagename, values[0]);
    else controller.HandleRoutingUpdate(messagename, values);
}
