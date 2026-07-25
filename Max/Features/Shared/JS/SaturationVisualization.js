var SaturationVisualizationOptions = {
    sensitivity: 0.9,
    smoothing: 0.85
};

function SaturationVisualization() {
    this.value = 0.0;
}

SaturationVisualization.prototype.Clamp = function(value) {
    return Math.max(0.0, Math.min(1.0, Number(value)));
};

SaturationVisualization.prototype.MapSensitivity = function(value) {
    var normalized = this.Clamp(value);
    var sensitivity = this.Clamp(SaturationVisualizationOptions.sensitivity);
    var exponent = 1.0 - 0.92 * sensitivity;
    return Math.pow(normalized, exponent);
};

SaturationVisualization.prototype.Update = function(value) {
    var target = this.MapSensitivity(value);
    var smoothing = SaturationVisualizationOptions.smoothing;
    this.value = this.value * smoothing + target * (1.0 - smoothing);
    return this.value;
};
