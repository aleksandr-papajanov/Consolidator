function SpectrumGeometry() {
}

SpectrumGeometry.prototype.Clamp = function(value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, value));
};

SpectrumGeometry.prototype.PlotBottom = function(height) {
    return Math.max(1, height - spectrumOptions.bottomPadding);
};

SpectrumGeometry.prototype.PlotTop = function() {
    return spectrumOptions.controlHeight + spectrumOptions.controlPadding;
};

SpectrumGeometry.prototype.FrequencyToX = function(frequency, width) {
    var settings = spectrumOptions;
    var minimum = settings.minimumFrequencyHz;
    var maximum = settings.maximumFrequencyHz;
    var ratio = Math.log(this.Clamp(frequency, minimum, maximum) / minimum) / Math.log(maximum / minimum);
    return ratio * width;
};

SpectrumGeometry.prototype.BinToX = function(index, count, width) {
    if (count < 2) return 0;
    var settings = spectrumOptions;
    var ratio = index / (count - 1);
    return this.FrequencyToX(settings.minimumFrequencyHz * Math.pow(settings.maximumFrequencyHz / settings.minimumFrequencyHz, ratio), width);
};

SpectrumGeometry.prototype.DbToY = function(value, plotBottom) {
    var settings = spectrumOptions;
    var scaleDb = settings.scaleDb;
    var ratio = (scaleDb - this.Clamp(value, -scaleDb, scaleDb)) / (scaleDb * 2);
    return this.PlotTop() + ratio * (plotBottom - this.PlotTop());
};

SpectrumGeometry.prototype.PointToFrequency = function(x, width) {
    var settings = spectrumOptions;
    var ratio = this.Clamp(x / Math.max(1, width), 0, 1);
    return settings.minimumFrequencyHz * Math.pow(settings.maximumFrequencyHz / settings.minimumFrequencyHz, ratio);
};

SpectrumGeometry.prototype.PointToGain = function(y, plotBottom) {
    var settings = spectrumOptions;
    var ratio = this.Clamp((y - this.PlotTop()) / Math.max(1, plotBottom - this.PlotTop()), 0, 1);
    return settings.scaleDb - ratio * settings.scaleDb * 2;
};

var spectrumGeometry = new SpectrumGeometry();
