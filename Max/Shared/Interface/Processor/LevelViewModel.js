include("ProcessorTelemetryOptions.js");

function LevelViewModel() {
    this.minimumDb = ProcessorTelemetryOptions.levels.minimumDb;
    this.maximumDb = ProcessorTelemetryOptions.levels.maximumDb;
    this.targetDb = this.ClampDb(ProcessorTelemetryOptions.levels.defaultTargetDb);
    this.levelDb = this.minimumDb;
    this.smoothedDb = this.minimumDb;
    this.peakDb = this.minimumDb;
    this.hasLevel = false;
    this.energyHistory = [];
    this.totalEnergy = 0.0;
    this.averagedMilliseconds = 0;
    this.normalizedValue = 0.0;
    this.normalizedSmoothedValue = 0.0;
    this.normalizedPeakValue = 0.0;
    this.hasNormalizedValue = false;
}

LevelViewModel.prototype.ClampDb = function(value) {
    return Math.max(this.minimumDb, Math.min(this.maximumDb, Number(value)));
};

LevelViewModel.prototype.SetTargetDb = function(value) {
    this.targetDb = this.ClampDb(value);
};

LevelViewModel.prototype.Reset = function() {
    this.levelDb = this.minimumDb;
    this.smoothedDb = this.minimumDb;
    this.peakDb = this.minimumDb;
    this.hasLevel = false;
    this.energyHistory = [];
    this.totalEnergy = 0.0;
    this.averagedMilliseconds = 0;
    this.normalizedValue = 0.0;
    this.normalizedSmoothedValue = 0.0;
    this.normalizedPeakValue = 0.0;
    this.hasNormalizedValue = false;
};

LevelViewModel.prototype.SetLevelDb = function(value) {
    var numericValue = Number(value);
    if (!isFinite(numericValue)) return;
    var timestamp = new Date().getTime();
    this.levelDb = this.ClampDb(numericValue);
    var energy = Math.pow(10.0, this.levelDb / 10.0);
    if (!this.hasLevel) {
        this.smoothedDb = this.levelDb;
        this.peakDb = this.levelDb;
        this.hasLevel = true;
        this.energyHistory = [{ timestamp: timestamp, energy: energy }];
        this.totalEnergy = energy;
        return;
    }
    this.energyHistory.push({ timestamp: timestamp, energy: energy });
    this.totalEnergy += energy;
    this.TrimEnergyHistory(timestamp);
    this.smoothedDb = 10.0 * Math.log(
        Math.max(1.0e-20, this.totalEnergy / this.energyHistory.length)
    ) / Math.LN10;
    this.peakDb = Math.max(
        this.levelDb,
        this.peakDb - ProcessorTelemetryOptions.levels.peakReleaseDb
    );
};

LevelViewModel.prototype.TrimEnergyHistory = function(timestamp) {
    var minimumTimestamp = timestamp
        - ProcessorTelemetryOptions.levels.averagingMilliseconds;
    while (this.energyHistory.length > 1
        && this.energyHistory[0].timestamp < minimumTimestamp) {
        this.totalEnergy -= this.energyHistory.shift().energy;
    }
    this.averagedMilliseconds = this.energyHistory.length < 2
        ? 0
        : timestamp - this.energyHistory[0].timestamp;
};

LevelViewModel.prototype.ValueForDb = function(valueDb) {
    var differenceDb = valueDb - this.targetDb;
    var availableRange = differenceDb < 0
        ? this.targetDb - this.minimumDb
        : this.maximumDb - this.targetDb;
    if (availableRange <= 0) return 0;
    return Math.max(-1.0, Math.min(1.0, differenceDb / availableRange));
};

LevelViewModel.prototype.SetNormalizedValue = function(value) {
    var numericValue = Number(value);
    if (!isFinite(numericValue)) return;
    this.normalizedValue = Math.max(0.0, Math.min(1.0, numericValue));
    if (!this.hasNormalizedValue) {
        this.normalizedSmoothedValue = this.normalizedValue;
        this.normalizedPeakValue = this.normalizedValue;
        this.hasNormalizedValue = true;
        return;
    }
    this.normalizedSmoothedValue = this.normalizedSmoothedValue
        * ProcessorTelemetryOptions.normalized.smoothing
        + this.normalizedValue * (1.0 - ProcessorTelemetryOptions.normalized.smoothing);
    this.normalizedPeakValue = Math.max(
        this.normalizedValue,
        this.normalizedPeakValue - ProcessorTelemetryOptions.normalized.peakRelease
    );
};

LevelViewModel.prototype.Value = function() {
    return this.hasLevel ? this.ValueForDb(this.smoothedDb) : 0.0;
};

LevelViewModel.prototype.PeakValue = function() {
    return this.hasLevel ? this.ValueForDb(this.peakDb) : 0.0;
};

LevelViewModel.prototype.SmoothedValue = function() {
    return this.hasLevel ? this.ValueForDb(this.smoothedDb) : 0.0;
};

LevelViewModel.prototype.SmoothedDb = function() {
    return this.hasLevel
        && this.averagedMilliseconds >= ProcessorTelemetryOptions.levels.minimumAveragingMilliseconds
        ? this.smoothedDb
        : null;
};

LevelViewModel.prototype.RawLevelDb = function() {
    return this.hasLevel ? this.levelDb : null;
};

LevelViewModel.prototype.PeakNormalizedValue = function() {
    return this.normalizedPeakValue;
};

LevelViewModel.prototype.SmoothedNormalizedValue = function() {
    return this.normalizedSmoothedValue;
};

LevelViewModel.prototype.HasNormalizedValue = function() {
    return this.hasNormalizedValue;
};
