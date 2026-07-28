function LiveApiInitializer(callback, owner, retryMilliseconds) {
    this.callback = callback;
    this.owner = owner;
    this.retryMilliseconds = Math.max(
        1, Number(retryMilliseconds) || 50);
    this.completed = false;
    this.task = new Task(this.TryInitialize, this);
}

LiveApiInitializer.prototype.Start = function() {
    if (!this.completed) this.TryInitialize();
};

LiveApiInitializer.prototype.TryInitialize = function() {
    if (this.completed) return;
    try {
        this.completed = this.callback.call(this.owner) === true;
    } catch (error) {
        this.completed = false;
    }
    if (!this.completed) this.task.schedule(this.retryMilliseconds);
};
