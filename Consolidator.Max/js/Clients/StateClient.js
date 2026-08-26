var MAX_BATCH_SIZE = 16;

// Absolute write/reset transport. Authoritative UI state belongs to
// TargetStateClient; this client deliberately has no state cache or readers.
function StateClient(protocol) {
    this.protocol = protocol;
    this.responses = {};

    protocol.on("state_begin", this.handleResponseBegin.bind(this));
    protocol.on("state_entry", this.handleResponseEntry.bind(this));
    protocol.on("state_done", this.handleResponseDone.bind(this));
}

StateClient.prototype.setFor = function (instanceId, path, value, callback, transactionId) {
    return this.setManyFor(
        instanceId,
        [{ path: path, value: value }],
        callback,
        transactionId);
};

StateClient.prototype.setManyFor = function (instanceId, entries, callback, transactionId) {
    if (instanceId === undefined || instanceId === null) {
        throw new Error("StateClient.setManyFor requires an instanceId.");
    }
    if (entries.length > MAX_BATCH_SIZE) {
        throw new Error("State batch cannot exceed 16 entries.");
    }
    var coalescingTransactionId = typeof callback === "function"
        ? 0 : transactionId || 0;
    var body = [
        String(instanceId),
        String(coalescingTransactionId),
        entries.length
    ];
    entries.forEach(function (entry) {
        body.push("entry");
        body = body.concat(this.encodePath(entry.path));
        body.push("value", this.encodeValue(entry.path, entry.value));
    }, this);
    return this.protocol.request("write", body, callback);
};

StateClient.prototype.resetFor = function (instanceId, path, callback, transactionId) {
    return this.protocol.request(
        "reset",
        [String(instanceId), String(transactionId || 0)].concat(this.encodePath(path)),
        callback
    );
};

StateClient.prototype.encodePath = function (path) {
    var parts = Array.isArray(path) ? path : path.split(".");
    return parts.map(function (part, index) {
        if (!/^\d+$/.test(String(part))) return part;
        var value = parseInt(part, 10);
        return index > 0 && parts[index - 1] === "bank" ? value + 1 : value;
    });
};

StateClient.prototype.encodeValue = function (path, value) {
    var text = Array.isArray(path) ? path.join(".") : path;
    if (/(^|\.)bank\.[0-6]\.group$/.test(text) && value === null) {
        return "none";
    }
    return value;
};

StateClient.prototype.decodeValue = function (path, value) {
    if (value === "none") return null;
    return value;
};

StateClient.prototype.decodeOptional = function (value) {
    return value === "none" ? undefined : value;
};

StateClient.prototype.handleResponseBegin = function (args) {
    if (args.length !== 6) return;
    var requestId = String(args[2]);
    this.responses[requestId] = {
        instanceId: String(args[3]),
        truncated: Number(args[4]) === 1,
        expectedCount: Number(args[5]),
        entries: [],
        invalid: false
    };
};

StateClient.prototype.handleResponseEntry = function (args) {
    var response = this.responses[String(args[2])];
    if (!response || args.length < 12) return;
    if (Number(args[4]) !== response.entries.length) {
        response.invalid = true;
        return;
    }
    var path = args.slice(5, -6).join(".");
    response.entries.push({
        path: path,
        value: this.decodeValue(path, args[args.length - 6]),
        status: this.decodeOptional(args[args.length - 5]),
        physicalMin: this.decodeOptional(args[args.length - 4]),
        physicalMax: this.decodeOptional(args[args.length - 3]),
        min: this.decodeOptional(args[args.length - 2]),
        max: this.decodeOptional(args[args.length - 1]),
        instanceId: String(args[3])
    });
};

StateClient.prototype.handleResponseDone = function (args) {
    if (args.length !== 4) return;
    var requestId = String(args[2]);
    var response = this.responses[requestId];
    delete this.responses[requestId];
    if (!response || String(args[3]) !== response.instanceId ||
            response.invalid ||
            response.entries.length !== response.expectedCount) {
        this.protocol.complete(requestId, {
            entries: [],
            error: "malformed_state_response"
        });
        return;
    }
    this.protocol.complete(requestId, {
        entries: response.entries,
        truncated: response.truncated,
        error: response.truncated ? "state_response_truncated" : null
    });
};

StateClient.prototype.destroy = function () {
    this.protocol = null;
    this.responses = {};
};
