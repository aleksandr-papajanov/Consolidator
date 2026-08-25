function TargetStateClient(protocol, state) {
    this.protocol = protocol;
    this.state = state;
    this.target = null;
    this.pendingTarget = null;
    this.cache = {};
    this.subscribers = {};
    this.statusSubscribers = [];
    this.snapshots = {};
    this.error = null;
    this.applyingSnapshot = false;
    protocol.on("target_state_begin", this.handleBegin.bind(this));
    protocol.on("target_state_entry", this.handleEntry.bind(this));
    protocol.on("target_state_done", this.handleDone.bind(this));
    protocol.on("state_changed", this.handleChanged.bind(this));
}

TargetStateClient.prototype.selectTarget = function (instanceId, bankId, callback) {
    var self = this;
    this.pendingTarget = { instanceId: String(instanceId), bankId: Number(bankId) };
    this.error = null;
    this.notifyStatus();
    return this.protocol.request(
        "observe_target",
        [String(instanceId), Number(bankId)],
        function (response) {
            if (response && response.error) {
                self.pendingTarget = null;
                self.error = response.error;
                self.notifyStatus();
            }
            if (callback) callback(response);
        }
    );
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
    return { ready: Boolean(this.target) && !this.pendingTarget,
        loading: Boolean(this.pendingTarget), target: this.target,
        error: this.error };
};

TargetStateClient.prototype.notifyStatus = function () {
    var status = this.status();
    this.statusSubscribers.slice().forEach(function (listener) { listener(status); });
};

TargetStateClient.prototype.handleBegin = function (args) {
    this.snapshots[String(args[2])] = {
        instanceId: String(args[3]), bankId: Number(args[4]),
        expected: Number(args[5]), entries: [], invalid: false
    };
};

TargetStateClient.prototype.handleEntry = function (args) {
    var snapshot = this.snapshots[String(args[2])];
    if (!snapshot || Number(args[3]) !== snapshot.entries.length) return;
    snapshot.entries.push(this.decodeEntry(args[4], args.slice(5), snapshot.instanceId));
};

TargetStateClient.prototype.handleDone = function (args) {
    var requestId = String(args[2]);
    var snapshot = this.snapshots[requestId];
    delete this.snapshots[requestId];
    var current = snapshot && this.pendingTarget &&
        snapshot.instanceId === this.pendingTarget.instanceId &&
        snapshot.bankId === this.pendingTarget.bankId;
    if (!snapshot || snapshot.entries.length !== snapshot.expected) {
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
    this.cache = {};
    this.applyingSnapshot = true;
    try {
        snapshot.entries.forEach(function (entry) {
            this.cache[entry.path] = entry;
            this.notify(entry);
        }, this);
    } finally {
        this.applyingSnapshot = false;
    }
    this.notifyStatus();
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
    this.snapshots = {};
};
