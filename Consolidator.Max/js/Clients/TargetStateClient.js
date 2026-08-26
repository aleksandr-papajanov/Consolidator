function TargetStateClient(protocol, state) {
    this.protocol = protocol;
    this.state = state;
    this.target = null;
    this.pendingTarget = null;
    this.cache = {};
    this.subscribers = {};
    this.statusSubscribers = [];
    this.error = null;
    this.applyingSnapshot = false;
    this.generation = 0;
    this.targetTransitionBeginListeners = [];
    this.targetTransitionDoneListeners = [];
    this.targetSnapshotCompletedListeners = [];
    protocol.on("target_state_snapshot", this.handleSnapshot.bind(this));
    protocol.on("state_changed", this.handleChanged.bind(this));
}

TargetStateClient.prototype.selectTarget = function (instanceId, bankId, callback) {
    var self = this;
    var generation = this.generation + 1;
    this.generation = generation;
    this.beginTargetTransition();
    this.pendingTarget = {
        instanceId: String(instanceId),
        bankId: Number(bankId),
        generation: generation,
        requestId: null
    };
    this.error = null;
    this.notifyStatus();
    var requestId = this.protocol.request(
        "observe_target",
        [String(instanceId), Number(bankId) + 1],
        function (response) {
            var current = self.pendingTarget &&
                self.pendingTarget.generation === generation;
            if (response && response.error) {
                if (current) {
                    self.pendingTarget = null;
                    self.error = response.error;
                    self.notifyStatus();
                    self.completeTargetTransition(generation);
                }
            }
            if (callback) callback(response);
        }
    );
    if (this.pendingTarget && this.pendingTarget.generation === generation) {
        this.pendingTarget.requestId = requestId;
    }
    return requestId;
};

TargetStateClient.prototype.beginTargetTransition = function () {
    this.targetTransitionBeginListeners.slice().forEach(function (listener) {
        listener();
    });
};

TargetStateClient.prototype.completeTargetTransition = function (generation) {
    if (generation !== this.generation) {
        return;
    }
    this.targetTransitionDoneListeners.slice().forEach(function (listener) {
        listener();
    });
};

TargetStateClient.prototype.onTargetTransitionBegin = function (callback) {
    this.targetTransitionBeginListeners.push(callback);
    var self = this;
    return function () {
        self.targetTransitionBeginListeners = self.targetTransitionBeginListeners.filter(
            function (listener) { return listener !== callback; });
    };
};

TargetStateClient.prototype.onTargetTransitionDone = function (callback) {
    this.targetTransitionDoneListeners.push(callback);
    var self = this;
    return function () {
        self.targetTransitionDoneListeners = self.targetTransitionDoneListeners.filter(
            function (listener) { return listener !== callback; });
    };
};

TargetStateClient.prototype.onTargetSnapshotCompleted = function (callback) {
    this.targetSnapshotCompletedListeners.push(callback);
    var self = this;
    return function () {
        self.targetSnapshotCompletedListeners = self.targetSnapshotCompletedListeners.filter(
            function (listener) { return listener !== callback; });
    };
};

TargetStateClient.prototype.set = function (path, value, callback, transactionId) {
    if (!this.target) return;
    this.state.setFor(this.target.instanceId,
        this.absolutePath(path), value, callback, transactionId);
};

TargetStateClient.prototype.setMany = function (entries, callback, transactionId) {
    if (!this.target) return;
    this.state.setManyFor(this.target.instanceId, entries.map(function (entry) {
        return { path: this.absolutePath(entry.path), value: entry.value };
    }, this), callback, transactionId);
};

TargetStateClient.prototype.reset = function (path, callback, transactionId) {
    if (!this.target) return;
    this.state.resetFor(this.target.instanceId,
        this.absolutePath(path), callback, transactionId);
};

TargetStateClient.prototype.absolutePath = function (path) {
    if (path.indexOf("equalizer.bank.") === 0 &&
            !/^equalizer\.bank\.\d+\./.test(path)) {
        return "equalizer.bank." + this.target.bankId + "." +
            path.substring("equalizer.bank.".length);
    }
    if (path.indexOf("equalizer.filter.") === 0) {
        return "equalizer.bank." + this.target.bankId + "." +
            path.substring("equalizer.".length);
    }
    return path;
};

TargetStateClient.prototype.subscribe = function (path, callback, immediate) {
    var self = this;
    if (!this.subscribers[path]) this.subscribers[path] = [];
    this.subscribers[path].push(callback);
    if (immediate && this.cache[path]) callback(this.cache[path]);
    return function () {
        self.subscribers[path] = (self.subscribers[path] || []).filter(
            function (listener) { return listener !== callback; });
    };
};

TargetStateClient.prototype.subscribeStatus = function (callback, immediate) {
    var self = this;
    this.statusSubscribers.push(callback);
    if (immediate) callback(this.status());
    return function () {
        self.statusSubscribers = self.statusSubscribers.filter(
            function (listener) { return listener !== callback; });
    };
};

TargetStateClient.prototype.status = function () {
    return { ready: Boolean(this.target),
        loading: Boolean(this.pendingTarget),
        targetTransitionPending: Boolean(this.pendingTarget), target: this.target,
        error: this.error };
};

TargetStateClient.prototype.notifyStatus = function () {
    var status = this.status();
    this.statusSubscribers.slice().forEach(function (listener) { listener(status); });
};

TargetStateClient.prototype.handleSnapshot = function (args) {
    var requestId = String(args[2]);
    var entryCount = Number(args[5]);
    var entrySize = 6;
    var snapshot = {
        instanceId: String(args[3]), bankId: Number(args[4]) - 1,
        expected: entryCount, entries: []
    };
    if (!isFinite(entryCount) || entryCount < 0 ||
            args.length !== 6 + entryCount * entrySize) {
        snapshot.invalid = true;
    }
    for (var index = 0; !snapshot.invalid && index < entryCount; index += 1) {
        var offset = 6 + index * entrySize;
        snapshot.entries.push(this.decodeEntry(
            args[offset],
            [args[offset + 1], "ready"].concat(args.slice(offset + 2, offset + entrySize)),
            snapshot.instanceId));
    }
    var current = snapshot && this.pendingTarget &&
        this.pendingTarget.requestId === requestId &&
        this.pendingTarget.generation === this.generation;
    if (snapshot.invalid || snapshot.entries.length !== snapshot.expected) {
        if (current) {
            this.pendingTarget = null;
            this.error = "malformed_target_state";
            this.notifyStatus();
            this.completeTargetTransition(this.generation);
        }
        this.protocol.complete(requestId, { error: "malformed_target_state" });
        return;
    }
    if (!current) {
        this.protocol.complete(requestId, { stale: true, error: null });
        return;
    }
    this.target = { instanceId: snapshot.instanceId, bankId: snapshot.bankId };
    this.pendingTarget = null;
    this.error = null;
    var nextCache = {};
    this.applyingSnapshot = true;
    try {
        snapshot.entries.forEach(function (entry) {
            nextCache[entry.path] = entry;
        }, this);
        this.cache = nextCache;
        snapshot.entries.forEach(function (entry) {
            this.notify(entry);
        }, this);
        this.notifyStatus();
    } finally {
        this.applyingSnapshot = false;
    }
    this.targetSnapshotCompletedListeners.slice().forEach(function (listener) {
        listener();
    });
    this.completeTargetTransition(this.generation);
    this.protocol.complete(requestId, { entries: snapshot.entries, error: null });
};

TargetStateClient.prototype.handleChanged = function (args) {
    if (!this.target || this.pendingTarget) return;
    var entry = this.decodeEntry(args[1], args.slice(2), this.target.instanceId);
    this.cache[entry.path] = entry;
    this.notify(entry);
};

TargetStateClient.prototype.decodeEntry = function (path, values, instanceId) {
    return { path: String(path), value: values[0], status: values[1],
        physicalMin: values[2] === "none" ? undefined : values[2],
        physicalMax: values[3] === "none" ? undefined : values[3],
        min: values[4] === "none" ? undefined : values[4],
        max: values[5] === "none" ? undefined : values[5],
        instanceId: instanceId };
};

TargetStateClient.prototype.notify = function (entry) {
    (this.subscribers[entry.path] || []).slice().forEach(function (listener) {
        listener(entry);
    });
};

TargetStateClient.prototype.destroy = function () {
    this.applyingSnapshot = false;
    this.cache = {};
    this.subscribers = {};
    this.statusSubscribers = [];
    this.targetTransitionBeginListeners = [];
    this.targetTransitionDoneListeners = [];
    this.targetSnapshotCompletedListeners = [];
};
