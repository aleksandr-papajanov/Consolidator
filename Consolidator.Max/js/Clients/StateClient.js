var MAX_BATCH_SIZE = 16;

function StateClient(protocol) {
    this.protocol = protocol;
    this.cache = {};
    this.subscribers = {};
    this.responses = {};

    protocol.on("state_begin", this.handleBegin.bind(this));
    protocol.on("state_entry", this.handleEntry.bind(this));
    protocol.on("state_done", this.handleDone.bind(this));
    protocol.on("action_done", this.handleActionDone.bind(this));
    protocol.on("error", this.handleError.bind(this));
}

StateClient.prototype.get = function (path) {
    return this.getFor(undefined, path);
};

StateClient.prototype.getFor = function (instanceId, path) {
    var bucket = this.cache[String(instanceId === undefined ? "default" : instanceId)];
    return bucket ? bucket[path] : undefined;
};

StateClient.prototype.fetch = function (path, callback) {
    return this.fetchMany([path], function (response) {
        if (!callback) {
            return;
        }
        if (response.error) {
            callback(undefined, response);
            return;
        }
        var own = response.byInstance[String(response.sourceInstanceId)];
        callback(own ? own[path] : undefined, null);
    });
};

StateClient.prototype.fetchMany = function (paths, callback) {
    if (paths.length > MAX_BATCH_SIZE) {
        throw new Error("State batch cannot exceed 16 entries.");
    }
    var body = [paths.length];
    for (var index = 0; index < paths.length; index += 1) {
        body.push("query");
        body = body.concat(this.encodePath(paths[index]));
    }
    return this.protocol.request("read", body, callback);
};

StateClient.prototype.set = function (path, value, callback) {
    return this.setMany([{ path: path, value: value }], callback);
};

StateClient.prototype.setMany = function (entries, callback) {
    if (entries.length > MAX_BATCH_SIZE) {
        throw new Error("State batch cannot exceed 16 entries.");
    }
    var body = [entries.length];
    for (var index = 0; index < entries.length; index += 1) {
        body.push("entry");
        body = body.concat(this.encodePath(entries[index].path));
        body.push(
            "value",
            this.encodeValue(entries[index].path, entries[index].value)
        );
    }
    return this.protocol.request("write", body, callback);
};

StateClient.prototype.reset = function (path, callback) {
    return this.protocol.request("reset", this.encodePath(path), callback);
};

StateClient.prototype.subscribe = function (path, callback, immediate) {
    return this.subscribeFor(undefined, path, callback, immediate);
};

StateClient.prototype.subscribeFor = function (instanceId, path, callback, immediate) {
    var self = this;
    this.addSubscriber(instanceId, path, callback);

    if (immediate) {
        var current = this.getFor(instanceId, path);
        if (current !== undefined) {
            callback(current);
        }
    }

    return function () {
        self.unsubscribeFor(instanceId, path, callback);
    };
};

StateClient.prototype.unsubscribe = function (path, callback) {
    this.removeSubscriber(undefined, path, callback);
};

StateClient.prototype.unsubscribeFor = function (instanceId, path, callback) {
    this.removeSubscriber(instanceId, path, callback);
};

StateClient.prototype.handleBegin = function (args) {
    var requestId = String(args[2]);
    this.responses[requestId] = {
        sourceInstanceId: args[3],
        truncated: args[4] === 1,
        expectedCount: args[5],
        entries: [],
        byInstance: {}
    };
};

StateClient.prototype.handleEntry = function (args) {
    var requestId = String(args[2]);
    var response = this.responses[requestId];
    if (!response) {
        return;
    }
    var path = this.decodePath(args.slice(5, -6));
    var entry = {
        path: path,
        value: this.decodeValue(path, args[args.length - 6]),
        status: this.decodeOptional(args[args.length - 5]),
        physicalMin: this.decodeOptional(args[args.length - 4]),
        physicalMax: this.decodeOptional(args[args.length - 3]),
        min: this.decodeOptional(args[args.length - 2]),
        max: this.decodeOptional(args[args.length - 1]),
        instanceId: args[3]
    };
    response.entries.push(entry);
    if (!response.byInstance[String(entry.instanceId)]) {
        response.byInstance[String(entry.instanceId)] = {};
    }
    response.byInstance[String(entry.instanceId)][path] = entry;
    this.publish(path, entry, response.sourceInstanceId);
};

StateClient.prototype.handleDone = function (args) {
    var requestId = String(args[2]);
    var response = this.responses[requestId] || {
        entries: [],
        byInstance: {}
    };
    delete this.responses[requestId];
    this.protocol.complete(requestId, response);
};

StateClient.prototype.handleActionDone = function (args) {
    this.protocol.complete(String(args[2]), {
        instanceId: args[3],
        status: args[4]
    });
};

StateClient.prototype.handleError = function (args) {
    var requestId = String(args[2]);
    delete this.responses[requestId];
};

StateClient.prototype.publish = function (path, entry, sourceInstanceId) {
    var instanceKey = String(entry.instanceId);
    if (!this.cache[instanceKey]) {
        this.cache[instanceKey] = {};
    }
    this.cache[instanceKey][path] = entry;
    this.notify(instanceKey, path, entry);

    if (instanceKey === String(sourceInstanceId)) {
        if (!this.cache.default) {
            this.cache.default = {};
        }
        this.cache.default[path] = entry;
        this.notify("default", path, entry);
    }
};

StateClient.prototype.notify = function (instanceId, path, entry) {
    var key = instanceId + ":" + path;
    var listeners = this.subscribers[key] || [];
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](entry);
    }
};

StateClient.prototype.addSubscriber = function (instanceId, path, callback) {
    var key = String(instanceId === undefined ? "default" : instanceId) + ":" + path;
    if (!this.subscribers[key]) {
        this.subscribers[key] = [];
    }
    this.subscribers[key].push(callback);
};

StateClient.prototype.removeSubscriber = function (instanceId, path, callback) {
    var key = String(instanceId === undefined ? "default" : instanceId) + ":" + path;
    var listeners = this.subscribers[key] || [];
    this.subscribers[key] = listeners.filter(function (listener) {
        return listener !== callback;
    });
};

StateClient.prototype.encodePath = function (path) {
    return path.split(".").map(function (part) {
        return /^\d+$/.test(part) ? parseInt(part, 10) : part;
    });
};

StateClient.prototype.encodeValue = function (path, value) {
    if (/^bank\.[1-7]\.group$/.test(path) && value === null) {
        return "none";
    }
    return value;
};

StateClient.prototype.decodePath = function (atoms) {
    return atoms.join(".");
};

StateClient.prototype.decodeValue = function (path, value) {
    if (value === "none") {
        return null;
    }
    if (path === "selected_bank" &&
        typeof value === "string" &&
        /^bank[1-7]$/.test(value)) {
        return parseInt(value.substring(4), 10);
    }
    return value;
};

StateClient.prototype.decodeOptional = function (value) {
    return value === "none" ? undefined : value;
};
