function panelDebug(message) {
    if (typeof post === "function") post("[panel-debug] target " + message + "\\n");
}

class TargetStateClient
{
    constructor(protocol, state)
    {
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
        this.targetSnapshotBatchBeginListeners = [];
        this.targetSnapshotBatchEndListeners = [];
        protocol.on("target_state_snapshot", this.handleSnapshot.bind(this));
        protocol.on("state_changed", this.handleChanged.bind(this));
    }
    
    selectTarget(instanceId, bankId, snapshotContext, callback)
    {
        panelDebug("request instance=" + instanceId + " bank=" + bankId +
            " context=" + snapshotContext);
        let generation = this.generation + 1;
        this.generation = generation;
        this.beginTargetTransition();
        this.pendingTarget = {
            instanceId: String(instanceId),
            bankId: Number(bankId),
            snapshotContext: String(snapshotContext || "equalizer"),
            generation: generation,
            requestId: null
        };
        this.error = null;
        this.notifyStatus();
        let requestId = this.protocol.request(
            "observe_target",
            [String(instanceId), Number(bankId), String(snapshotContext || "equalizer")],
            (response) => {
                let current = this.pendingTarget &&
                    this.pendingTarget.generation === generation;
                if (response && response.error) {
                    if (current) {
                        this.pendingTarget = null;
                        this.error = response.error;
                        this.notifyStatus();
                        this.completeTargetTransition(generation);
                    }
                }
                if (callback) callback(response);
            }
        );
        if (this.pendingTarget && this.pendingTarget.generation === generation) {
            this.pendingTarget.requestId = requestId;
        }
        return requestId;
    }
    
    beginTargetTransition()
    {
        this.targetTransitionBeginListeners.slice().forEach((listener) => {
            listener();
        });
    }
    
    completeTargetTransition(generation)
    {
        if (generation !== this.generation) {
            return;
        }
        this.targetTransitionDoneListeners.slice().forEach((listener) => {
            listener();
        });
    }
    
    onTargetTransitionBegin(callback)
    {
        this.targetTransitionBeginListeners.push(callback);
        return () => {
            this.targetTransitionBeginListeners = this.targetTransitionBeginListeners.filter(
                (listener) => { return listener !== callback; });
        };
    }
    
    onTargetTransitionDone(callback)
    {
        this.targetTransitionDoneListeners.push(callback);
        return () => {
            this.targetTransitionDoneListeners = this.targetTransitionDoneListeners.filter(
                (listener) => { return listener !== callback; });
        };
    }
    
    onTargetSnapshotCompleted(callback)
    {
        this.targetSnapshotCompletedListeners.push(callback);
        return () => {
            this.targetSnapshotCompletedListeners = this.targetSnapshotCompletedListeners.filter(
                (listener) => { return listener !== callback; });
        };
    }
    
    onTargetSnapshotBatchBegin(callback)
    {
        this.targetSnapshotBatchBeginListeners.push(callback);
        return () => {
            this.targetSnapshotBatchBeginListeners =
                this.targetSnapshotBatchBeginListeners.filter((listener) => {
                    return listener !== callback;
                });
        };
    }
    
    onTargetSnapshotBatchEnd(callback)
    {
        this.targetSnapshotBatchEndListeners.push(callback);
        return () => {
            this.targetSnapshotBatchEndListeners =
                this.targetSnapshotBatchEndListeners.filter((listener) => {
                    return listener !== callback;
                });
        };
    }
    
    set(path, value, callback, transactionId)
    {
        if (!this.target) return;
        this.state.setFor(this.target.instanceId,
            this.absolutePath(path), value, callback, transactionId);
    }
    
    setMany(entries, callback, transactionId)
    {
        if (!this.target) return;
        this.state.setManyFor(this.target.instanceId, entries.map((entry) => {
            return { path: this.absolutePath(entry.path), value: entry.value };
        }, this), callback, transactionId);
    }
    
    reset(path, callback, transactionId)
    {
        if (!this.target) return;
        this.state.resetFor(this.target.instanceId,
            this.absolutePath(path), callback, transactionId);
    }
    
    absolutePath(path)
    {
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
    }
    
    subscribe(path, callback, immediate)
    {
        if (!this.subscribers[path]) this.subscribers[path] = [];
        this.subscribers[path].push(callback);
        if (immediate && this.cache[path]) callback(this.cache[path]);
        return () => {
            this.subscribers[path] = (this.subscribers[path] || []).filter(
                (listener) => { return listener !== callback; });
        };
    }
    
    subscribeStatus(callback, immediate)
    {
        this.statusSubscribers.push(callback);
        if (immediate) callback(this.status());
        return () => {
            this.statusSubscribers = this.statusSubscribers.filter(
                (listener) => { return listener !== callback; });
        };
    }
    
    status()
    {
        return { ready: Boolean(this.target),
            loading: Boolean(this.pendingTarget),
            targetTransitionPending: Boolean(this.pendingTarget), target: this.target,
            error: this.error };
    }
    
    notifyStatus()
    {
        let status = this.status();
        this.statusSubscribers.slice().forEach((listener) => { listener(status); });
    }
    
    handleSnapshot(args)
    {
        let requestId = String(args[2]);
        let snapshotContext = String(args[5]);
        let entryCount = Number(args[6]);
        let entrySize = 6;
        let snapshot = {
                instanceId: String(args[3]), bankId: Number(args[4]),
                snapshotContext: snapshotContext, expected: entryCount, entries: []
        };
        panelDebug("received request=" + requestId + " instance=" +
            snapshot.instanceId + " bank=" + snapshot.bankId + " context=" +
            snapshot.snapshotContext);
        if (!isFinite(entryCount) || entryCount < 0 ||
                args.length !== 7 + entryCount * entrySize) {
            snapshot.invalid = true;
        }
        for (let index = 0; !snapshot.invalid && index < entryCount; index += 1) {
            let offset = 7 + index * entrySize;
            snapshot.entries.push(this.decodeEntry(
                args[offset],
                [args[offset + 1], "ready"].concat(args.slice(offset + 2, offset + entrySize)),
                snapshot.instanceId));
        }
        let current = snapshot && this.pendingTarget &&
            this.pendingTarget.requestId === requestId &&
            this.pendingTarget.generation === this.generation &&
            this.pendingTarget.instanceId === snapshot.instanceId &&
            this.pendingTarget.bankId === snapshot.bankId &&
            this.pendingTarget.snapshotContext === snapshot.snapshotContext;
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
            panelDebug("stale snapshot request=" + requestId);
            this.protocol.complete(requestId, { stale: true, error: null });
            return;
        }
        this.target = {
            instanceId: snapshot.instanceId,
            bankId: snapshot.bankId,
            snapshotContext: snapshot.snapshotContext
        };
        this.pendingTarget = null;
        this.error = null;
        let nextCache = {};
        this.applyingSnapshot = true;
        this.targetSnapshotBatchBeginListeners.slice().forEach((listener) => {
            listener();
        });
        try {
            snapshot.entries.forEach((entry) => {
                nextCache[entry.path] = entry;
            }, this);
            this.cache = nextCache;
            snapshot.entries.forEach((entry) => {
                this.notify(entry);
            }, this);
            this.notifyStatus();
            this.targetSnapshotCompletedListeners.slice().forEach((listener) => {
                listener(snapshot);
            });
        } finally {
            this.applyingSnapshot = false;
            this.targetSnapshotBatchEndListeners.slice().forEach((listener) => {
                listener();
            });
        }
        this.completeTargetTransition(this.generation);
        this.protocol.complete(requestId, {
            entries: snapshot.entries,
            snapshotContext: snapshot.snapshotContext,
            error: null
        });
    }
    
    handleChanged(args)
    {
        if (!this.target || this.pendingTarget) return;
        let entry = this.decodeEntry(args[1], args.slice(2), this.target.instanceId);
        this.cache[entry.path] = entry;
        this.notify(entry);
    }
    
    decodeEntry(path, values, instanceId)
    {
        return { path: String(path), value: values[0], status: values[1],
            physicalMin: values[2] === "none" ? undefined : values[2],
            physicalMax: values[3] === "none" ? undefined : values[3],
            min: values[4] === "none" ? undefined : values[4],
            max: values[5] === "none" ? undefined : values[5],
            instanceId: instanceId };
    }
    
    notify(entry)
    {
        (this.subscribers[entry.path] || []).slice().forEach((listener) => {
            listener(entry);
        });
    }
    
    destroy()
    {
        this.applyingSnapshot = false;
        this.cache = {};
        this.subscribers = {};
        this.statusSubscribers = [];
        this.targetTransitionBeginListeners = [];
        this.targetTransitionDoneListeners = [];
        this.targetSnapshotCompletedListeners = [];
        this.targetSnapshotBatchBeginListeners = [];
        this.targetSnapshotBatchEndListeners = [];
    }
}

module.exports = {
    TargetStateClient: TargetStateClient
};
