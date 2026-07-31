include("ProcessorTelemetryOptions.js");

function SaturationViewModel() {
    this.value = 0.0;
    this.rawValue = 0.0;
    this.smoothedRawValue = 0.0;
    this.hasValue = false;
}

SaturationViewModel.prototype.Clamp = function(value) {
    return Math.max(0.0, Math.min(1.0, Number(value)));
};

SaturationViewModel.prototype.MapSensitivity = function(value) {
    var normalized = this.Clamp(value);
    var sensitivity = this.Clamp(ProcessorTelemetryOptions.saturation.sensitivity);
    var exponent = 1.0 - 0.92 * sensitivity;
    return Math.pow(normalized, exponent);
};

SaturationViewModel.prototype.Update = function(value) {
    this.rawValue = this.Clamp(value);
    if (!this.hasValue) {
        this.smoothedRawValue = this.rawValue;
        this.hasValue = true;
    } else {
        this.smoothedRawValue = this.smoothedRawValue
            * ProcessorTelemetryOptions.saturation.smoothing
            + this.rawValue * (1.0 - ProcessorTelemetryOptions.saturation.smoothing);
    }
    var target = this.MapSensitivity(this.rawValue);
    var smoothing = ProcessorTelemetryOptions.saturation.smoothing;
    this.value = this.value * smoothing + target * (1.0 - smoothing);
    return this.value;
};

SaturationViewModel.prototype.Reset = function() {
    this.value = 0.0;
    this.rawValue = 0.0;
    this.smoothedRawValue = 0.0;
    this.hasValue = false;
};

SaturationViewModel.prototype.SmoothedRawValue = function() {
    return this.hasValue ? this.smoothedRawValue : null;
};
