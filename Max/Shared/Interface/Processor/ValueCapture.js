function ValueCapture(mode, maximumDurationMilliseconds, completionCallback, completionContext) {
    this.mode = String(mode || "linear");
    this.active = false;
    this.weightedSum = 0.0;
    this.durationMilliseconds = 0.0;
    this.lastTimestamp = 0;
    this.lastValue = null;
    this.maximumDurationMilliseconds = Math.max(0, Number(maximumDurationMilliseconds) || 0);
    this.completionCallback = completionCallback || null;
    this.completionContext = completionContext || null;
    this.completionTask = new Task(this.CompleteAutomatically, this);
}

ValueCapture.prototype.Begin = function() {
    this.active = true;
    this.weightedSum = 0.0;
    this.durationMilliseconds = 0.0;
    this.lastTimestamp = 0;
    this.lastValue = null;
    this.completionTask.cancel();
    if (this.maximumDurationMilliseconds > 0) {
        this.completionTask.schedule(this.maximumDurationMilliseconds);
    }
};

ValueCapture.prototype.Observe = function(value) {
    if (!this.active) return;
    var numericValue = Number(value);
    if (!isFinite(numericValue)) return;
    var timestamp = new Date().getTime();
    var weightedValue = this.mode === "energyDb"
        ? Math.pow(10.0, numericValue / 10.0)
        : numericValue;
    this.AppendDuration(timestamp);
    this.lastTimestamp = timestamp;
    this.lastValue = weightedValue;
};

ValueCapture.prototype.AppendDuration = function(timestamp) {
    if (this.lastValue === null || timestamp <= this.lastTimestamp) return;
    var duration = timestamp - this.lastTimestamp;
    this.weightedSum += this.lastValue * duration;
    this.durationMilliseconds += duration;
};

ValueCapture.prototype.Finish = function() {
    if (!this.active) return null;
    this.active = false;
    this.completionTask.cancel();
    if (this.lastValue === null) return null;
    this.AppendDuration(new Date().getTime());
    var average = this.durationMilliseconds > 0.0
        ? this.weightedSum / this.durationMilliseconds
        : this.lastValue;
    return this.mode === "energyDb"
        ? 10.0 * Math.log(Math.max(1.0e-20, average)) / Math.LN10
        : average;
};

ValueCapture.prototype.CompleteAutomatically = function() {
    var value = this.Finish();
    if (this.completionCallback) {
        this.completionCallback.call(this.completionContext, value);
    }
};

ValueCapture.prototype.Cancel = function() {
    this.active = false;
    this.weightedSum = 0.0;
    this.durationMilliseconds = 0.0;
    this.lastTimestamp = 0;
    this.lastValue = null;
    this.completionTask.cancel();
};

ValueCapture.prototype.Dispose = function() {
    this.Cancel();
    this.completionCallback = null;
    this.completionContext = null;
};
