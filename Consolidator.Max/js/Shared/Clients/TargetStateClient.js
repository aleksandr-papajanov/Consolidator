const {
    decodeTargetStateEntry,
    decodeTargetStateSnapshot
} = require("./TargetStateSnapshotDecoder.js");
const { TargetStateSubscriptions } = require("./TargetStateSubscriptions.js");

class TargetStateClient
{
    constructor(protocol, state)
    {
        this.protocol = protocol;
        this.state = state;
        this.target = null;
        this.pendingTarget = null;
        this.cache = {};
        this.error = null;
        this.applyingSnapshot = false;
        this.generation = 0;
        this.subscriptions = new TargetStateSubscriptions();
        this.unsubscribeProtocol = [
            protocol.on("target_state_snapshot", (args) => this.handleSnapshot(args)),
            protocol.on("state_changed", (args) => this.handleChanged(args))
        ];
    }

    selectTarget(instanceId, bankId, snapshotContext, callback)
    {
        const generation = ++this.generation;
        const context = String(snapshotContext || "equalizer");
        this.subscriptions.notify("transitionBegin");
        this.pendingTarget = {
            instanceId: String(instanceId),
            bankId: Number(bankId),
            snapshotContext: context,
            generation: generation,
            requestId: null
        };
        this.error = null;
        this.notifyStatus();
        const requestId = this.protocol.request(
            "observe_target",
            [String(instanceId), Number(bankId), context],
            (response) => {
                const current = this.isCurrentGeneration(generation);
                if (response && response.error && current)
                {
                    this.pendingTarget = null;
                    this.error = response.error;
                    this.notifyStatus();
                    this.completeTargetTransition(generation);
                }
                if (callback)
                {
                    callback(response);
                }
            }
        );
        if (this.isCurrentGeneration(generation))
        {
            this.pendingTarget.requestId = requestId;
        }
        return requestId;
    }

    isCurrentGeneration(generation)
    {
        return Boolean(this.pendingTarget &&
            this.pendingTarget.generation === generation);
    }

    completeTargetTransition(generation)
    {
        if (generation === this.generation)
        {
            this.subscriptions.notify("transitionDone");
        }
    }

    onTargetTransitionBegin(callback)
    {
        return this.subscriptions.subscribeTransitionBegin(callback);
    }

    onTargetTransitionDone(callback)
    {
        return this.subscriptions.subscribeTransitionDone(callback);
    }

    onTargetSnapshotCompleted(callback)
    {
        return this.subscriptions.subscribeSnapshotCompleted(callback);
    }

    onTargetSnapshotBatchBegin(callback)
    {
        return this.subscriptions.subscribeBatchBegin(callback);
    }

    onTargetSnapshotBatchEnd(callback)
    {
        return this.subscriptions.subscribeBatchEnd(callback);
    }

    set(path, value, callback, transactionId, scope)
    {
        if (this.target)
        {
            this.state.set(this.relativePath(path), value, callback, transactionId, scope);
        }
    }

    setMany(entries, callback, transactionId, scope)
    {
        if (this.target)
        {
            this.state.setMany(entries.map((entry) => {
                return { path: this.relativePath(entry.path), value: entry.value };
            }), callback, transactionId, scope);
        }
    }

    reset(path, callback, transactionId, scope)
    {
        if (this.target)
        {
            this.state.reset(this.relativePath(path), callback, transactionId, scope);
        }
    }

    relativePath(path)
    {
        if (path.indexOf("equalizer.bank.") === 0 &&
                !/^equalizer\.bank\.\d+\./.test(path))
        {
            return path;
        }
        if (path.indexOf("equalizer.filter.") === 0)
        {
            return "equalizer.bank." + path.substring("equalizer.".length);
        }
        return path;
    }

    subscribe(path, callback, immediate)
    {
        const unsubscribe = this.subscriptions.subscribePath(path, callback);
        if (immediate && this.cache[path])
        {
            callback(this.cache[path]);
        }
        return unsubscribe;
    }

    subscribeStatus(callback, immediate)
    {
        const unsubscribe = this.subscriptions.subscribeStatus(callback);
        if (immediate)
        {
            callback(this.status());
        }
        return unsubscribe;
    }

    status()
    {
        return {
            ready: Boolean(this.target),
            loading: Boolean(this.pendingTarget),
            targetTransitionPending: Boolean(this.pendingTarget),
            target: this.target,
            error: this.error
        };
    }

    notifyStatus()
    {
        this.subscriptions.notify("status", this.status());
    }

    handleSnapshot(args)
    {
        const requestId = String(args[2]);
        const snapshot = decodeTargetStateSnapshot(args);
        const current = this.isCurrentSnapshot(requestId, snapshot);
        if (snapshot.invalid || snapshot.entries.length !== snapshot.expected)
        {
            if (current)
            {
                this.pendingTarget = null;
                this.error = "malformed_target_state";
                this.notifyStatus();
                this.completeTargetTransition(this.generation);
            }
            this.protocol.complete(requestId, { error: "malformed_target_state" });
            return;
        }
        if (!current)
        {
            this.protocol.complete(requestId, { stale: true, error: null });
            return;
        }

        this.applySnapshot(snapshot);
        this.completeTargetTransition(this.generation);
        this.protocol.complete(requestId, {
            entries: snapshot.entries,
            snapshotContext: snapshot.snapshotContext,
            error: null
        });
    }

    isCurrentSnapshot(requestId, snapshot)
    {
        const pending = this.pendingTarget;
        return Boolean(pending && pending.requestId === requestId &&
            pending.generation === this.generation &&
            pending.instanceId === snapshot.instanceId &&
            pending.bankId === snapshot.bankId &&
            pending.snapshotContext === snapshot.snapshotContext);
    }

    applySnapshot(snapshot)
    {
        this.target = {
            instanceId: snapshot.instanceId,
            bankId: snapshot.bankId,
            snapshotContext: snapshot.snapshotContext
        };
        this.pendingTarget = null;
        this.error = null;
        this.applyingSnapshot = true;
        this.subscriptions.notify("batchBegin");
        try
        {
            this.cache = {};
            snapshot.entries.forEach((entry) => {
                this.cache[entry.path] = entry;
            });
            snapshot.entries.forEach((entry) => this.subscriptions.notifyPath(entry));
            this.notifyStatus();
            this.subscriptions.notify("snapshotCompleted", snapshot);
        }
        finally
        {
            this.applyingSnapshot = false;
            this.subscriptions.notify("batchEnd");
        }
    }

    handleChanged(args)
    {
        if (!this.target || this.pendingTarget)
        {
            return;
        }

        const entry = decodeTargetStateEntry(
            args[1],
            args.slice(2),
            this.target.instanceId
        );
        this.cache[entry.path] = entry;
        this.subscriptions.notifyPath(entry);
    }

    destroy()
    {
        this.unsubscribeProtocol.forEach((unsubscribe) => unsubscribe());
        this.unsubscribeProtocol = [];
        this.applyingSnapshot = false;
        this.cache = {};
        this.target = null;
        this.pendingTarget = null;
        this.error = null;
        this.subscriptions.clear();
        this.protocol = null;
        this.state = null;
    }
}

module.exports = {
    TargetStateClient: TargetStateClient
};
