class TargetStateSubscriptions
{
    constructor()
    {
        this.paths = {};
        this.status = [];
        this.transitionBegin = [];
        this.transitionDone = [];
        this.snapshotCompleted = [];
        this.batchBegin = [];
        this.batchEnd = [];
    }

    subscribePath(path, callback)
    {
        return this.subscribeArray(this.paths, String(path), callback);
    }

    subscribeStatus(callback)
    {
        return this.subscribeList("status", callback);
    }

    subscribeTransitionBegin(callback)
    {
        return this.subscribeList("transitionBegin", callback);
    }

    subscribeTransitionDone(callback)
    {
        return this.subscribeList("transitionDone", callback);
    }

    subscribeSnapshotCompleted(callback)
    {
        return this.subscribeList("snapshotCompleted", callback);
    }

    subscribeBatchBegin(callback)
    {
        return this.subscribeList("batchBegin", callback);
    }

    subscribeBatchEnd(callback)
    {
        return this.subscribeList("batchEnd", callback);
    }

    notifyPath(entry)
    {
        this.notifyList(this.paths[entry.path] || [], entry);
    }

    notify(name, value)
    {
        this.notifyList(this[name], value);
    }

    subscribeArray(collection, key, callback)
    {
        collection[key] = collection[key] || [];
        collection[key].push(callback);
        return () => {
            collection[key] = (collection[key] || []).filter((listener) => {
                return listener !== callback;
            });
        };
    }

    subscribeList(name, callback)
    {
        this[name].push(callback);
        return () => {
            this[name] = this[name].filter((listener) => listener !== callback);
        };
    }

    notifyList(listeners, value)
    {
        listeners.slice().forEach((listener) => listener(value));
    }

    clear()
    {
        this.paths = {};
        this.status = [];
        this.transitionBegin = [];
        this.transitionDone = [];
        this.snapshotCompleted = [];
        this.batchBegin = [];
        this.batchEnd = [];
    }
}

module.exports = {
    TargetStateSubscriptions: TargetStateSubscriptions
};
