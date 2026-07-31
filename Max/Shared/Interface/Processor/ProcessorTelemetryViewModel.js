include("LevelViewModel.js");
include("SaturationViewModel.js");
include("ProcessorTelemetryOptions.js");

function ProcessorTelemetryViewModel(resetCallback, resetContext) {
    this.inputLevel = new LevelViewModel();
    this.outputLevel = new LevelViewModel();
    this.compressorLevel = new LevelViewModel();
    this.saturatorLevel = new LevelViewModel();
    this.reduction = new LevelViewModel();
    this.saturation = new SaturationViewModel();
    this.resetCallback = resetCallback;
    this.resetContext = resetContext;
    this.resetTask = new Task(this.Reset, this);
    this.resetFadeTask = new Task(this.AdvanceReset, this);
    this.resetScale = 0.0;
    this.resetDisplay = null;
}

ProcessorTelemetryViewModel.prototype.Update = function(values) {
    if (!values || values.length < 9) return false;
    this.resetFadeTask.cancel();
    this.resetScale = 0.0;
    this.resetDisplay = null;
    this.inputLevel.SetLevelDb(values[4]);
    this.outputLevel.SetLevelDb(values[6]);
    this.compressorLevel.SetLevelDb(values[7]);
    this.saturatorLevel.SetLevelDb(values[8]);
    this.reduction.SetNormalizedValue(
        Math.max(
            0.0,
            Math.min(
                1.0,
                -Number(values[0])
                    / ProcessorTelemetryOptions.telemetry.compressorReductionMaximumDb
            )
        )
    );
    this.saturation.Update(values[1]);
    this.resetTask.cancel();
    this.resetTask.schedule(
        ProcessorTelemetryOptions.telemetry.timeoutMilliseconds
    );
    return true;
};

ProcessorTelemetryViewModel.prototype.SetStageTarget = function(stage, valueDb) {
    var level = this.LevelForStage(stage);
    if (level) level.SetTargetDb(valueDb);
};

ProcessorTelemetryViewModel.prototype.LevelForStage = function(stage) {
    return String(stage) === "input" ? this.inputLevel : this.outputLevel;
};

ProcessorTelemetryViewModel.prototype.LevelForProcessor = function(device) {
    return String(device) === "compressor"
        ? this.compressorLevel
        : this.saturatorLevel;
};

ProcessorTelemetryViewModel.prototype.CaptureResetDisplay = function() {
    return {
        input: this.DisplayStageLevel("input"),
        output: this.DisplayStageLevel("output"),
        compressor: this.DisplayProcessorLevel("compressor"),
        saturator: this.DisplayProcessorLevel("saturator"),
        reduction: this.DisplayReduction(),
        saturation: this.DisplaySaturation()
    };
};

ProcessorTelemetryViewModel.prototype.DisplayLevelValues = function(level) {
    return { peak: level.PeakValue(), smoothed: level.SmoothedValue() };
};

ProcessorTelemetryViewModel.prototype.DisplayResetValues = function(values) {
    if (this.resetScale <= 0 || !this.resetDisplay) return values;
    return {
        peak: values.peak * this.resetScale,
        smoothed: values.smoothed * this.resetScale
    };
};

ProcessorTelemetryViewModel.prototype.DisplayStageLevel = function(stage) {
    var level = this.LevelForStage(stage);
    if (this.resetScale > 0 && this.resetDisplay) {
        return {
            peak: this.resetDisplay[stage].peak * this.resetScale,
            smoothed: this.resetDisplay[stage].smoothed * this.resetScale
        };
    }
    return this.DisplayLevelValues(level);
};

ProcessorTelemetryViewModel.prototype.DisplayProcessorLevel = function(device) {
    var level = this.LevelForProcessor(device);
    if (this.resetScale > 0 && this.resetDisplay) {
        return {
            peak: this.resetDisplay[device].peak * this.resetScale,
            smoothed: this.resetDisplay[device].smoothed * this.resetScale
        };
    }
    return this.DisplayLevelValues(level);
};

ProcessorTelemetryViewModel.prototype.DisplayReduction = function() {
    if (this.resetScale > 0 && this.resetDisplay) {
        return this.resetDisplay.reduction * this.resetScale;
    }
    return this.reduction.PeakNormalizedValue();
};

ProcessorTelemetryViewModel.prototype.DisplaySaturation = function() {
    return this.resetScale > 0 && this.resetDisplay
        ? this.resetDisplay.saturation * this.resetScale
        : this.saturation.value;
};

ProcessorTelemetryViewModel.prototype.Reset = function() {
    this.resetFadeTask.cancel();
    this.resetDisplay = this.CaptureResetDisplay();
    this.inputLevel.Reset();
    this.outputLevel.Reset();
    this.compressorLevel.Reset();
    this.saturatorLevel.Reset();
    this.reduction.Reset();
    this.saturation.Reset();
    this.resetScale = 1.0;
    if (this.resetCallback) this.resetCallback.call(this.resetContext);
    if (this.HasResetDisplay()) {
        this.resetFadeTask.schedule(
            ProcessorTelemetryOptions.telemetry.resetIntervalMilliseconds
        );
    } else {
        this.resetScale = 0.0;
        this.resetDisplay = null;
    }
};

ProcessorTelemetryViewModel.prototype.HasResetDisplay = function() {
    if (!this.resetDisplay) return false;
    return Math.abs(this.resetDisplay.input.peak) > 0.001
        || Math.abs(this.resetDisplay.input.smoothed) > 0.001
        || Math.abs(this.resetDisplay.output.peak) > 0.001
        || Math.abs(this.resetDisplay.output.smoothed) > 0.001
        || Math.abs(this.resetDisplay.compressor.peak) > 0.001
        || Math.abs(this.resetDisplay.compressor.smoothed) > 0.001
        || Math.abs(this.resetDisplay.saturator.peak) > 0.001
        || Math.abs(this.resetDisplay.saturator.smoothed) > 0.001
        || this.resetDisplay.reduction > 0.001
        || this.resetDisplay.saturation > 0.001;
};

ProcessorTelemetryViewModel.prototype.AdvanceReset = function() {
    this.resetScale *= ProcessorTelemetryOptions.telemetry.resetSmoothing;
    if (this.resetScale <= ProcessorTelemetryOptions.telemetry.resetMinimumScale) {
        this.resetScale = 0.0;
        this.resetDisplay = null;
    }
    if (this.resetCallback) this.resetCallback.call(this.resetContext);
    if (this.resetScale > 0.0) {
        this.resetFadeTask.schedule(
            ProcessorTelemetryOptions.telemetry.resetIntervalMilliseconds
        );
    }
};
