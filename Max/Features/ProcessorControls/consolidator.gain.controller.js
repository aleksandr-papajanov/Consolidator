autowatch = 1;
inlets = 2;
outlets = 4;
include("../Shared/JS/TargetLevelIndicator.js");

// Inlet 0: Host snapshots, set_gain <0..1>, set_target <0..1>, rms <dB>,
// processor_telemetry <compressorReductionDb> <saturationNonlinearRatio>
// <saturationLevelDeltaDb> <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb>
// <compressorOutputDb> <saturatorOutputDb>,
// enabled <0|1>.
// Inlet 1: DialControl output <ring-index> <0..1>.
// Outlet 0: Host command gain.set_parameter.
// Outlet 1: DialControl commands set <ring-index> <0..1>,
// indicator <ring-index> <-1..1>, enabled <0|1>.
// Outlet 2: diagnostics error <code>.
// Outlet 3: input target level target_level <absoluteDb>.

function GainController(stage) {
    this.stage = stage === "output" ? "output" : "input";
    this.target = this.NormalizeTarget(TargetLevelIndicatorOptions.defaultTargetDb);
    this.levelIndicator = new TargetLevelIndicator();
    this.definition = null;
    this.pendingGain = null;
    this.pendingSnapshotGainDb = null;
    this.requestId = 0;
}

GainController.prototype.ClampNormalized = function(value) {
    return Math.max(0.0, Math.min(1.0, Number(value)));
};

GainController.prototype.NormalizeTarget = function(valueDb) {
    return this.ClampNormalized(
        (Number(valueDb) - TargetLevelIndicatorOptions.minimumDb)
            / (
                TargetLevelIndicatorOptions.maximumDb
                    - TargetLevelIndicatorOptions.minimumDb
            )
    );
};

GainController.prototype.TargetDb = function() {
    return TargetLevelIndicatorOptions.minimumDb
        + this.target * (
            TargetLevelIndicatorOptions.maximumDb
                - TargetLevelIndicatorOptions.minimumDb
        );
};

GainController.prototype.SetGain = function(value) {
    outlet(1, "set", 1, this.ClampNormalized(value));
};

GainController.prototype.ToAbsoluteGain = function(value) {
    if (!this.definition) return null;
    var normalized = this.ClampNormalized(value);
    return this.definition.minimum
        + normalized * (this.definition.maximum - this.definition.minimum);
};

GainController.prototype.ToNormalizedGain = function(value) {
    if (!this.definition) return 0.5;
    return this.ClampNormalized(
        (Number(value) - this.definition.minimum)
            / (this.definition.maximum - this.definition.minimum)
    );
};

GainController.prototype.SendGain = function(value) {
    var absolute = this.ToAbsoluteGain(value);
    if (absolute === null) {
        this.pendingGain = this.ClampNormalized(value);
        return;
    }
    this.pendingGain = null;
    this.requestId += 1;
    outlet(0, "command", [
        1,
        "gain.controls",
        this.requestId,
        "gain.set_parameter",
        this.stage,
        absolute
    ]);
};

GainController.prototype.SetTarget = function(value) {
    this.target = this.ClampNormalized(value);
    this.levelIndicator.SetTargetDb(this.TargetDb());
    outlet(1, "set", 2, this.target);
    this.UpdateIndicator();
    if (this.stage === "input") outlet(3, "target_level", this.TargetDb());
};

GainController.prototype.SetRms = function(valueDb) {
    var value = Number(valueDb);
    if (!isFinite(value)) return;
    this.levelIndicator.SetLevelDb(value);
    this.UpdateIndicator();
};

GainController.prototype.UpdateIndicator = function() {
    outlet(1, "visualization", 1, "signed", this.levelIndicator.Value());
};

GainController.prototype.HandleDial = function(values) {
    if (values.length < 2) return;
    var ring = Number(values[0]);
    var value = this.ClampNormalized(values[1]);
    if (ring === 1) {
        this.SendGain(value);
        return;
    }
    if (ring === 2) {
        this.SetTarget(value);
    }
};

GainController.prototype.HandleTelemetry = function(values) {
    if (values.length < 7) return;
    this.SetRms(this.stage === "input" ? values[4] : values[6]);
};

GainController.prototype.Initialize = function() {
    this.SetTarget(this.target);
};

GainController.prototype.ReadParameter = function(values, position) {
    return {
        value: {
            name: String(values[position]),
            minimum: Number(values[position + 1]),
            maximum: Number(values[position + 2])
        },
        next: position + 5
    };
};

GainController.prototype.HandleProcessorDefinitions = function(values) {
    var count = Number(values[5]);
    var position = 6;
    var expectedId = this.stage === "input" ? "input_gain" : "output_gain";
    for (var index = 0; index < count; index++) {
        var id = String(values[position++]);
        var parameterCount = Number(values[position++]);
        for (var parameterIndex = 0; parameterIndex < parameterCount; parameterIndex++) {
            var decoded = this.ReadParameter(values, position);
            position = decoded.next;
            if (id === expectedId && decoded.value.name === "gain") {
                this.definition = decoded.value;
            }
        }
    }
    if (!this.definition) {
        outlet(2, "error", "missing_gain_definition", this.stage);
        return;
    }
    if (this.pendingSnapshotGainDb !== null) {
        this.SetGain(this.ToNormalizedGain(this.pendingSnapshotGainDb));
        this.pendingSnapshotGainDb = null;
    }
    if (this.pendingGain !== null) this.SendGain(this.pendingGain);
};

GainController.prototype.HandleDspSnapshot = function(values) {
    if (values.length < 32) return;
    var position = this.stage === "input" ? values.length - 32 : values.length - 1;
    if (!this.definition) {
        this.pendingSnapshotGainDb = Number(values[position]);
        return;
    }
    this.SetGain(this.ToNormalizedGain(values[position]));
};

GainController.prototype.HandleSnapshot = function(values) {
    if (values.length < 6 || String(values[0]) !== "snapshot") return;
    var store = String(values[3]);
    if (store === "processor_definitions") this.HandleProcessorDefinitions(values);
    else if (store === "dsp") this.HandleDspSnapshot(values);
};

var stageArgument = jsarguments.length > 1 ? String(jsarguments[1]) : "input";
var controller = new GainController(stageArgument);

function loadbang() {
    controller.Initialize();
}

function set_gain(value) {
    if (inlet === 0) controller.SetGain(value);
}

function set_target(value) {
    if (inlet === 0) controller.SetTarget(value);
}

function rms(value) {
    if (inlet === 0) controller.SetRms(value);
}

function processor_telemetry() {
    if (inlet === 0) controller.HandleTelemetry(arrayfromargs(arguments));
}

function snapshot() {
    if (inlet === 0) controller.HandleSnapshot(["snapshot"].concat(arrayfromargs(arguments)));
}

function event() {}
function status() {}

function enabled(value) {
    if (inlet === 0) outlet(1, "enabled", Number(value) !== 0 ? 1 : 0);
}

function list() {
    var values = arrayfromargs(arguments);
    if (inlet === 1) controller.HandleDial(values);
    else if (values.length && String(values[0]) === "snapshot") {
        controller.HandleSnapshot(values);
    }
}

function msg_float(value) {
    if (inlet === 1) controller.HandleDial([1, value]);
}

function msg_int(value) {
    if (inlet === 1) controller.HandleDial([1, value]);
}

function inletassist(index) {
    assist(index === 0
        ? "Commands: set_gain <0..1>, set_target <0..1>, rms <dB>, processor_telemetry <9 values>, enabled <0|1>"
        : "DialControl output: <ring-index> <0..1>");
}

function outletassist(index) {
    assist([
        "Host command: gain.set_parameter <input|output> <absoluteDb>",
        "DialControl commands: set, visualization, enabled",
        "Diagnostics: error <code>",
        "Input target level: target_level <absoluteDb>"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
