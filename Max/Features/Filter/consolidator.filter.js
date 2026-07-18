autowatch = 1;
inlets = 1;
outlets = 3;

include("../Shared/JS/Messages/MessageEnvelope.js");

function FilterEndpoint(slot) {
    this.slot = Number(slot) || 0;
    this.configured = false;
    this.ready = false;
}

FilterEndpoint.prototype.Configure = function() {
    this.configured = true;
    this.Send("system.status", {
        feature: "filter",
        filterId: this.slot,
        state: "ready"
    }, "bus.hub");
};

FilterEndpoint.prototype.Update = function(control, value) {
    if (!this.configured) return;
    var normalized = Number(value);
    if (!isFinite(normalized) || normalized < 0 || normalized > 1) {
        outlet(2, "error", "invalid_filter_control");
        return;
    }
    this.Send("filter.control", {
        filterId: this.slot,
        control: String(control),
        value: normalized
    });
};

FilterEndpoint.prototype.Reset = function() {
    if (this.configured) this.Send("filter.reset", { filterId: this.slot });
};

FilterEndpoint.prototype.HandleEnvelope = function(dictionaryName) {
    var message = MessageEnvelope.fromMaxDictionary(dictionaryName);
    if (!message || message.target !== "filter" || message.type !== "filter.state") return;
    if (!message.payload || Number(message.payload.filterId) !== this.slot) return;
    var values = message.payload.normalizedValues instanceof Array
        ? message.payload.normalizedValues.slice(0) : [];
    values.push(Number(message.payload.bypass) === 1 ? 1 : 0);
    outlet(1, ["status", "values"].concat(values));
    if (!this.ready) {
        this.ready = true;
        outlet(1, "status", "ready");
    }
};

FilterEndpoint.prototype.Send = function(type, payload, target) {
    var message = MessageEnvelope.create(type, target || "eq.storage", payload, "filter");
    var dictionary = message.toMaxDictionary();
    outlet(0, "message", dictionary.name);
};

var filterEndpoint = new FilterEndpoint(jsarguments[1]);

function inletassist(index) {
    assist(index === 0
        ? "Local: configure, update <control> <0..1>, reset; bus: filter.state"
        : "");
}

function outletassist(index) {
    var descriptions = [
        "message <filter.control|filter.reset|system.status envelope>",
        "status ready or values <normalized parameters> <bypass>",
        "error <code>"
    ];
    assist(descriptions[index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);

function configure() { filterEndpoint.Configure(); }
function update(control, value) { filterEndpoint.Update(control, value); }
function reset() { filterEndpoint.Reset(); }
function message(dictionaryName) { filterEndpoint.HandleEnvelope(dictionaryName); }
