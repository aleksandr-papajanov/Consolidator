function LatestValueDispatcher(intervalMilliseconds, callback, owner) {
    this.intervalMilliseconds = Math.max(1, Number(intervalMilliseconds) || 16);
    this.callback = callback;
    this.owner = owner;
    this.pending = {};
    this.scheduled = false;
    this.task = new Task(this.Flush, this);
}

LatestValueDispatcher.prototype.Enqueue = function(key, value) {
    this.pending[String(key)] = value;
    if (this.scheduled) return;
    this.scheduled = true;
    this.task.schedule(this.intervalMilliseconds);
};

LatestValueDispatcher.prototype.Flush = function() {
    this.scheduled = false;
    var pending = this.pending;
    this.pending = {};
    for (var key in pending) {
        if (pending.hasOwnProperty(key)) {
            this.callback.call(this.owner, pending[key]);
        }
    }
};

LatestValueDispatcher.prototype.Dispose = function() {
    this.task.cancel();
    this.pending = {};
    this.scheduled = false;
};
