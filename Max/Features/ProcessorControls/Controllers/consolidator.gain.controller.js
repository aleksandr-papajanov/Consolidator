autowatch = 1;
inlets = 2;
outlets = 5;
include("../../../Shared/Runtime/ControlControllerBase.js");
include("../../../Shared/Interface/Processor/ProcessorTelemetryViewModel.js");
include("../../../Shared/Interface/Processor/ValueCapture.js");
include("../../../Shared/Configuration/FilterDefinitions.js");

// Inlet 0: Host snapshots, set_gain <0..1>, set_target <0..1>,
// processor_telemetry <compressorReductionDb> <saturationNonlinearRatio>
// <saturationLevelDeltaDb> <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb>
// <compressorOutputDb> <saturatorOutputDb>,
// enabled <0|1>.
// Inlet 1: DialControl output <ring-index> <0..1> or levelMatch <0|1>.
// Outlet 0: Host command gain.set_parameter.
// Outlet 1: DialControl commands set <ring-index> <0..1>,
// indicator <ring-index> <-1..1>, levelVisualization <ring-index> <peak> <smooth>,
// ringColor, clearRingColor, enabled <0|1>.
// Outlet 2: diagnostics error <code>.
// Outlet 3: input target level target_level <absoluteDb>.

function GainController(stage) {
    ControlControllerBase.call(this, "gain.controls", this.FlushGain, this);
    this.stage = stage === "output" ? "output" : "input";
    this.target = this.NormalizeTarget(
        ProcessorTelemetryOptions.levels.defaultTargetDb
    );
    this.telemetry = new ProcessorTelemetryViewModel(
        this.ResetTelemetryIndicator,
        this
    );
    var processorDefinitions = FilterDefinitionCatalog.Processors();
    this.definition = processorDefinitions[
        this.stage === "input" ? "input_gain" : "output_gain"
    ].parameters[0];
    this.pendingGain = null;
    this.pendingSnapshotGainDb = null;
    this.pendingLimits = null;
    this.currentGain = this.ToNormalizedGain(this.definition.defaultValue);
    this.levelMatchInProgress = false;
    this.levelMatchCapture = new ValueCapture(
        "energyDb",
        ProcessorTelemetryOptions.capture.maximumDurationMilliseconds,
        this.CompleteLevelMatch,
        this
    );
}

GainController.prototype = Object.create(ControlControllerBase.prototype);
GainController.prototype.constructor = GainController;

GainController.prototype.NormalizeTarget = function(valueDb) {
    return this.ClampNormalized(
        (Number(valueDb) - ProcessorTelemetryOptions.levels.minimumDb)
            / (
                ProcessorTelemetryOptions.levels.maximumDb
                    - ProcessorTelemetryOptions.levels.minimumDb
            )
    );
};

GainController.prototype.TargetDb = function() {
    return ProcessorTelemetryOptions.levels.minimumDb
        + this.target * (
            ProcessorTelemetryOptions.levels.maximumDb
                - ProcessorTelemetryOptions.levels.minimumDb
        );
};

GainController.prototype.SetGain = function(value) {
    this.currentGain = this.ClampNormalized(value);
    outlet(1, "set", 1, this.currentGain);
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
    var normalized = this.ClampNormalized(value);
    this.currentGain = normalized;
    var absolute = this.ToAbsoluteGain(normalized);
    if (absolute === null) {
        this.pendingGain = normalized;
        return;
    }
    this.pendingGain = null;
    this.parameterDispatcher.Enqueue(this.stage, {
        normalized: normalized,
        absolute: absolute
    });
};

GainController.prototype.FlushGain = function(update) {
    outlet(4, "processor_parameter_gesture",
        this.stage === "input" ? "input_gain" : "output_gain",
        "gain", update.normalized);
    this.SendCommand("gain.set_parameter", [this.stage, update.absolute]);
};

GainController.prototype.SetTarget = function(value) {
    this.target = this.ClampNormalized(value);
    this.telemetry.SetStageTarget(this.stage, this.TargetDb());
    if (this.stage === "input") outlet(3, "target_level", this.TargetDb());
};

GainController.prototype.ResetTelemetryIndicator = function() {
    this.UpdateIndicator();
};

GainController.prototype.UpdateIndicator = function() {
    var level = this.telemetry.DisplayStageLevel(this.stage);
    outlet(1, "levelVisualization", 1,
        level.peak,
        level.smoothed);
};

GainController.prototype.MatchLevel = function(measuredDb) {
    if (!this.definition) return;
    if (measuredDb === null) return;
    var currentDb = this.ToAbsoluteGain(this.currentGain);
    var targetDb = this.TargetDb();
    var nextDb = Math.max(
        this.definition.minimum,
        Math.min(this.definition.maximum, currentDb + targetDb - measuredDb)
    );
    var nextValue = this.ToNormalizedGain(nextDb);
    this.SetGain(nextValue);
    this.SendGain(nextValue);
};

GainController.prototype.HandleLevelMatch = function(value) {
    if (Number(value) !== 0) {
        if (this.levelMatchInProgress) return;
        this.levelMatchInProgress = true;
        outlet(1, "enabled", 0);
        this.levelMatchCapture.Begin();
        return;
    }
    this.CompleteLevelMatch(this.levelMatchCapture.Finish());
};

GainController.prototype.CompleteLevelMatch = function(measuredDb) {
    outlet(1, "levelMatch", 0);
    this.MatchLevel(measuredDb);
    if (!this.levelMatchInProgress) return;
    this.levelMatchInProgress = false;
    outlet(1, "enabled", 1);
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
    if (!this.telemetry.Update(values)) return;
    this.levelMatchCapture.Observe(
        this.telemetry.LevelForStage(this.stage).RawLevelDb()
    );
    this.UpdateIndicator();
};

GainController.prototype.Initialize = function() {
    this.telemetry.Reset();
    this.SetTarget(this.target);
    this.ApplyDefinition();
};

GainController.prototype.ApplyDefinition = function() {
    if (!this.definition) return;
    outlet(1, "displayRange", 1, this.definition.minimum,
        this.definition.maximum, 0, 1, " dB");
    outlet(1, "displayRange", 2,
        ProcessorTelemetryOptions.levels.minimumDb,
        ProcessorTelemetryOptions.levels.maximumDb, 0, 1, " dB");
    outlet(1, "step", 2, 0.05);
    outlet(1, "levelMatchEnabled", 1);
    outlet(1, "defaultValue", 1,
        this.ToNormalizedGain(this.definition.defaultValue));
    outlet(1, "defaultValue", 2,
        this.NormalizeTarget(ProcessorTelemetryOptions.levels.defaultTargetDb));
    if (this.pendingSnapshotGainDb !== null) {
        this.SetGain(this.ToNormalizedGain(this.pendingSnapshotGainDb));
        this.pendingSnapshotGainDb = null;
    }
    if (this.pendingGain !== null) this.SendGain(this.pendingGain);
    if (this.pendingLimits) {
        this.ApplyProcessorLimits(
            this.pendingLimits.device,
            this.pendingLimits.parameter,
            this.pendingLimits.minimum,
            this.pendingLimits.maximum);
    }
};

GainController.prototype.HandleProcessorSnapshot = function(values) {
    if (values.length < 29) return;
    var base = values.length - 29;
    var position = this.stage === "input" ? base : base + 28;
    if (!this.definition) {
        this.pendingSnapshotGainDb = Number(values[position]);
        return;
    }
    this.SetGain(this.ToNormalizedGain(values[position]));
};

GainController.prototype.SetLinkColor = function(linkId, red, green, blue, alpha) {
    if (String(linkId) === "-") {
        outlet(1, "clearRingColor", 1);
        return;
    }
    outlet(1, "ringColor", 1, Number(red), Number(green), Number(blue), Number(alpha));
};

GainController.prototype.HandleProcessorLimits = function(device, parameter, minimum, maximum) {
    this.pendingLimits = {
        device: device,
        parameter: parameter,
        minimum: minimum,
        maximum: maximum
    };
    this.ApplyProcessorLimits(device, parameter, minimum, maximum);
};

GainController.prototype.HandleProcessorPreview = function(device, parameter, absoluteValue) {
    var expectedDevice = this.stage === "input" ? "input_gain" : "output_gain";
    if (String(device) !== expectedDevice || String(parameter) !== "gain" ||
        !this.definition) return;
    this.SetGain(this.ToNormalizedGain(Number(absoluteValue)));
};

GainController.prototype.ApplyProcessorLimits = function(device, parameter, minimum, maximum) {
    var expectedDevice = this.stage === "input" ? "input_gain" : "output_gain";
    if (String(device) !== expectedDevice || String(parameter) !== "gain" || !this.definition) return;
    outlet(1, "limits", 1,
        this.ToNormalizedGain(minimum),
        this.ToNormalizedGain(maximum));
};

GainController.prototype.HandleSnapshot = function(values) {
    if (values.length < 6 || String(values[0]) !== "snapshot") return;
    var store = String(values[3]);
    if (store === "processor") this.HandleProcessorSnapshot(values);
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

function processor_telemetry() {
    if (inlet === 0) controller.HandleTelemetry(arrayfromargs(arguments));
}

function processor_limits(device, parameter, minimum, maximum) {
    if (inlet === 0) controller.HandleProcessorLimits(
        String(device), String(parameter), Number(minimum), Number(maximum));
}

function link_color(linkId, red, green, blue, alpha) {
    if (inlet === 0) controller.SetLinkColor(
        String(linkId), Number(red), Number(green), Number(blue), Number(alpha));
}

function processor_preview(device, parameter, absoluteValue) {
    if (inlet === 0) controller.HandleProcessorPreview(
        String(device), String(parameter), Number(absoluteValue));
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
    if (inlet === 1 && String(values[0]) === "levelMatch") {
        controller.HandleLevelMatch(values[1]);
    } else if (inlet === 1) controller.HandleDial(values);
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

// The scoped processor link channel is shared with detector controllers.
function detector_link_preview() {}

function levelMatch(value) {
    if (inlet === 1) controller.HandleLevelMatch(value);
}

function inletassist(index) {
    assist(index === 0
        ? "Commands: set_gain <0..1>, set_target <0..1>, processor_telemetry <9 values>, processor_limits <device> gain <absoluteMinimum> <absoluteMaximum>, processor_preview <input_gain|output_gain> gain <absoluteDb>, link_color <linkId|-> <rgba>, enabled <0|1>"
        : "DialControl output: <ring-index> <0..1> or levelMatch <0|1>");
}

function outletassist(index) {
    assist([
        "Host command: gain.set_parameter <input|output> <absoluteDb>",
        "DialControl commands: set, step, displayRange, levelMatchEnabled, visualization, ringColor, clearRingColor, enabled; target ring uses normalized step 0.05 (3 dB)",
        "Diagnostics: error <code>",
        "Input target level: target_level <absoluteDb>",
        "Live link gesture: processor_parameter_gesture <input_gain|output_gain> gain <normalizedValue>"
    ][index] || "");
}

setinletassist(-1, inletassist);
setoutletassist(-1, outletassist);
