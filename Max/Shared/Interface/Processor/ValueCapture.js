function ValueCapture(mode, maximumDurationMilliseconds, completionCallback, completionContext) {
    this.mode = String(mode || "linear");
    this.active = false;
    this.sum = 0.0;
    this.count = 0;
    this.maximumDurationMilliseconds = Math.max(0, Number(maximumDurationMilliseconds) || 0);
    this.completionCallback = completionCallback || null;
    this.completionContext = completionContext || null;
    this.completionTask = new Task(this.CompleteAutomatically, this);
}

ValueCapture.prototype.Begin = function() {
    this.active = true;
    this.sum = 0.0;
    this.count = 0;
    this.completionTask.cancel();
    if (this.maximumDurationMilliseconds > 0) {
        this.completionTask.schedule(this.maximumDurationMilliseconds);
    }
};

ValueCapture.prototype.Observe = function(value) {
    if (!this.active) return;
    var numericValue = Number(value);
    if (!isFinite(numericValue)) return;
    this.sum += this.mode === "energyDb"
        ? Math.pow(10.0, numericValue / 10.0)
        : numericValue;
    this.count += 1;
};

ValueCapture.prototype.Finish = function() {
    if (!this.active) return null;
    this.active = false;
    this.completionTask.cancel();
    if (this.count === 0) return null;
    var average = this.sum / this.count;
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
    this.sum = 0.0;
    this.count = 0;
    this.completionTask.cancel();
};
