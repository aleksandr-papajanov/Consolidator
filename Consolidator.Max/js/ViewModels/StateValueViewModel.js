function StateValueViewModel(state, path) {
    this.state = state;
    this.path = path;
    this.instanceId = undefined;
    this.value = undefined;
    this.minimum = undefined;
    this.maximum = undefined;
    this.physicalMinimum = undefined;
    this.physicalMaximum = undefined;
    this.status = undefined;
    this.loaded = false;
    this.enabled = true;
    this.loading = true;
    this.snapshotDirty = false;
    this.listeners = [];

    var self = this;
    this.unsubscribeState = state.subscribe(path, function (entry) {
        self.applyEntry(entry);
    }, true);
    this.unsubscribeStatus = state.subscribeStatus
        ? state.subscribeStatus(function (status) {
            var enabled = Boolean(status.ready);
            if (self.enabled === enabled) return;
            self.enabled = enabled;
            if (self.state.applyingSnapshot) {
                self.snapshotDirty = true;
                return;
            }
            self.notify();
        }, true)
        : null;
    this.unsubscribeSnapshotCompleted = state.onTargetSnapshotCompleted
        ? state.onTargetSnapshotCompleted(function () {
            self.completeSnapshot();
        })
        : null;
}

StateValueViewModel.prototype.applyEntry = function (entry) {
    if (!entry) {
        return;
    }
    if (this.loaded && this.entryIsCurrent(entry)) {
        return;
    }
    this.loaded = true;
    this.loading = false;
    this.value = entry.value;
    this.minimum = entry.min;
    this.maximum = entry.max;
    this.physicalMinimum = entry.physicalMin;
    this.physicalMaximum = entry.physicalMax;
    this.status = entry.status;
    this.instanceId = entry.instanceId;
    if (this.state.applyingSnapshot) {
        this.snapshotDirty = true;
        return;
    }
    this.notify();
};

StateValueViewModel.prototype.completeSnapshot = function () {
    if (!this.snapshotDirty) {
        return;
    }
    this.snapshotDirty = false;
    this.notify();
};

StateValueViewModel.prototype.set = function (value, callback, transactionId) {
    if (!this.enabled) {
        return;
    }
    this.state.set(this.path, value, callback, transactionId);
};

StateValueViewModel.prototype.entryIsCurrent = function (entry) {
    return this.value === entry.value &&
        this.minimum === entry.min && this.maximum === entry.max &&
        this.physicalMinimum === entry.physicalMin &&
        this.physicalMaximum === entry.physicalMax &&
        this.status === entry.status && this.instanceId === entry.instanceId;
};

StateValueViewModel.prototype.reset = function (transactionId) {
    if (!this.enabled) {
        return;
    }
    this.state.reset(this.path, undefined, transactionId);
};

StateValueViewModel.prototype.subscribe = function (callback, immediate) {
    var self = this;
    this.listeners.push(callback);

    if (immediate && this.loaded) {
        callback(this);
    }

    return function () {
        self.listeners = self.listeners.filter(function (listener) {
            return listener !== callback;
        });
    };
};

StateValueViewModel.prototype.notify = function () {
    for (var index = 0; index < this.listeners.length; index += 1) {
        this.listeners[index](this);
    }
};

StateValueViewModel.prototype.destroy = function () {
    if (this.unsubscribeState) {
        this.unsubscribeState();
        this.unsubscribeState = null;
    }
    if (this.unsubscribeStatus) {
        this.unsubscribeStatus();
        this.unsubscribeStatus = null;
    }
    if (this.unsubscribeSnapshotCompleted) {
        this.unsubscribeSnapshotCompleted();
        this.unsubscribeSnapshotCompleted = null;
    }
    this.listeners = [];
};
