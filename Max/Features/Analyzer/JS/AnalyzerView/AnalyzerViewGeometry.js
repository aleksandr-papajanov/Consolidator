function AnalyzerViewGeometry() {
}

AnalyzerViewGeometry.prototype.Clamp = function(value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, value));
};

AnalyzerViewGeometry.prototype.PlotBottom = function(height) {
    return Math.max(1, height - analyzerViewConfig.spectrum.bottomPadding);
};

AnalyzerViewGeometry.prototype.PlotTop = function() {
    return analyzerViewConfig.spectrum.controlHeight + analyzerViewConfig.spectrum.controlPadding;
};

AnalyzerViewGeometry.prototype.FrequencyToX = function(frequency, width) {
    var settings = analyzerViewConfig.spectrum;
    var minimum = settings.minimumFrequencyHz;
    var maximum = settings.maximumFrequencyHz;
    var ratio = Math.log(this.Clamp(frequency, minimum, maximum) / minimum) / Math.log(maximum / minimum);
    return ratio * width;
};

AnalyzerViewGeometry.prototype.BinToX = function(index, count, width) {
    if (count < 2) return 0;
    var settings = analyzerViewConfig.spectrum;
    var ratio = index / (count - 1);
    return this.FrequencyToX(settings.minimumFrequencyHz * Math.pow(settings.maximumFrequencyHz / settings.minimumFrequencyHz, ratio), width);
};

AnalyzerViewGeometry.prototype.DbToY = function(value, plotBottom) {
    var settings = analyzerViewConfig.spectrum;
    var scaleDb = settings.scaleDb;
    var ratio = (scaleDb - this.Clamp(value, -scaleDb, scaleDb)) / (scaleDb * 2);
    return this.PlotTop() + ratio * (plotBottom - this.PlotTop());
};

AnalyzerViewGeometry.prototype.PointToFrequency = function(x, width) {
    var settings = analyzerViewConfig.spectrum;
    var ratio = this.Clamp(x / Math.max(1, width), 0, 1);
    return settings.minimumFrequencyHz * Math.pow(settings.maximumFrequencyHz / settings.minimumFrequencyHz, ratio);
};

AnalyzerViewGeometry.prototype.PointToGain = function(y, plotBottom) {
    var settings = analyzerViewConfig.spectrum;
    var ratio = this.Clamp((y - this.PlotTop()) / Math.max(1, plotBottom - this.PlotTop()), 0, 1);
    return settings.scaleDb - ratio * settings.scaleDb * 2;
};

var analyzerViewGeometry = new AnalyzerViewGeometry();
