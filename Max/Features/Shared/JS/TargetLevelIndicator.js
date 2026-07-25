var TargetLevelIndicatorOptions = {
    minimumDb: -60.0,
    maximumDb: 0.0,
    defaultTargetDb: -18.0
};

function TargetLevelIndicator() {
    this.minimumDb = TargetLevelIndicatorOptions.minimumDb;
    this.maximumDb = TargetLevelIndicatorOptions.maximumDb;
    this.targetDb = this.ClampDb(TargetLevelIndicatorOptions.defaultTargetDb);
    this.levelDb = this.minimumDb;
}

TargetLevelIndicator.prototype.ClampDb = function(value) {
    return Math.max(this.minimumDb, Math.min(this.maximumDb, Number(value)));
};

TargetLevelIndicator.prototype.SetTargetDb = function(value) {
    this.targetDb = this.ClampDb(value);
};

TargetLevelIndicator.prototype.SetLevelDb = function(value) {
    var numericValue = Number(value);
    if (!isFinite(numericValue)) return;
    this.levelDb = this.ClampDb(numericValue);
};

TargetLevelIndicator.prototype.Value = function() {
    var differenceDb = this.levelDb - this.targetDb;
    var availableRange = differenceDb < 0
        ? this.targetDb - this.minimumDb
        : this.maximumDb - this.targetDb;
    if (availableRange <= 0) return 0;
    return Math.max(-1.0, Math.min(1.0, differenceDb / availableRange));
};
