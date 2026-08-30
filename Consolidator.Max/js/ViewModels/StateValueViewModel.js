class StateValueViewModel
{
    constructor(state, path)
    {
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
        this.unsubscribeState = state.subscribe(path, (entry) => {
            this.applyEntry(entry);
        }, true);
        this.unsubscribeStatus = state.subscribeStatus
            ? state.subscribeStatus((status) => {
                let enabled = Boolean(status.ready);
                if (this.enabled === enabled) return;
                this.enabled = enabled;
                if (this.state.applyingSnapshot) {
                    this.snapshotDirty = true;
                    return;
                }
                this.notify();
            }, true)
            : null;
        this.unsubscribeSnapshotCompleted = state.onTargetSnapshotCompleted
            ? state.onTargetSnapshotCompleted(() => {
                this.completeSnapshot();
            })
            : null;
    }
    
    applyEntry(entry)
    {
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
    }
    
    completeSnapshot()
    {
        if (!this.snapshotDirty) {
            return;
        }
        this.snapshotDirty = false;
        this.notify();
    }
    
    set(value, callback, transactionId)
    {
        if (!this.enabled) {
            return;
        }
        this.state.set(this.path, value, callback, transactionId);
    }
    
    entryIsCurrent(entry)
    {
        return this.value === entry.value &&
            this.minimum === entry.min && this.maximum === entry.max &&
            this.physicalMinimum === entry.physicalMin &&
            this.physicalMaximum === entry.physicalMax &&
            this.status === entry.status && this.instanceId === entry.instanceId;
    }
    
    reset(transactionId, scope)
    {
        if (!this.enabled) {
            return;
        }
        this.state.reset(this.path, undefined, transactionId, scope);
    }
    
    subscribe(callback, immediate)
    {
        this.listeners.push(callback);
    
        if (immediate && this.loaded) {
            callback(this);
        }
    
        return () => {
            this.listeners = this.listeners.filter((listener) => {
                return listener !== callback;
            });
        };
    }
    
    notify()
    {
        for (let index = 0; index < this.listeners.length; index += 1) {
            this.listeners[index](this);
        }
    }
    
    destroy()
    {
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
    }
}


module.exports = {
    StateValueViewModel: StateValueViewModel
};
