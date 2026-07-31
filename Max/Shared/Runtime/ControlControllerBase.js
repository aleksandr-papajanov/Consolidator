include("LatestValueDispatcher.js");

function ControlControllerBase(source, flushCallback, owner) {
    this.source = String(source);
    this.requestId = 0;
    this.parameterDispatcher = flushCallback
        ? new LatestValueDispatcher(16, flushCallback, owner || this)
        : null;
}

ControlControllerBase.prototype.ClampNormalized = function(value) {
    return Math.max(0.0, Math.min(1.0, Number(value)));
};

ControlControllerBase.prototype.SendCommand = function(name, fields) {
    this.requestId += 1;
    outlet(0, "command", [1, this.source, this.requestId, name].concat(fields || []));
};

ControlControllerBase.prototype.ToAbsolute = function(definition, normalized) {
    if (!definition) return null;
    var value = this.ClampNormalized(normalized);
    return definition.minimum + value * (definition.maximum - definition.minimum);
};

ControlControllerBase.prototype.ToNormalized = function(definition, absolute) {
    if (!definition || definition.maximum === definition.minimum) return 0.5;
    return this.ClampNormalized(
        (Number(absolute) - definition.minimum)
            / (definition.maximum - definition.minimum)
    );
};
