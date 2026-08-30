const MAX_BATCH_SIZE = 16;

class StateClient
{
    // Relative write/reset transport. Managed resolves the selected target and
    // bank from the source SelectionContext. Explicit targeting is topology-only.
    constructor(protocol, scope)
    {
        this.protocol = protocol;
        this.scope = scope || { mode: "local" };
        this.responses = {};
    
        protocol.on("state_begin", this.handleResponseBegin.bind(this));
        protocol.on("state_entry", this.handleResponseEntry.bind(this));
        protocol.on("state_done", this.handleResponseDone.bind(this));
    }
    
    set(path, value, callback, transactionId, scope)
    {
        return this.setMany(
            [{ path: path, value: value }],
            callback,
            transactionId,
            scope);
    }
    
    setMany(entries, callback, transactionId, scope)
    {
        return this.sendWrite(scope || this.scope.mode, null, entries, callback,
            transactionId);
    }

    setManyTopologyFor(instanceId, entries, callback, transactionId)
    {
        if (instanceId === undefined || instanceId === null) {
            throw new Error("Topology write requires an instanceId.");
        }
        return this.sendWrite("topology", instanceId, entries, callback,
            transactionId);
    }

    sendWrite(scope, instanceId, entries, callback, transactionId)
    {
        if (entries.length > MAX_BATCH_SIZE) {
            throw new Error("State batch cannot exceed 16 entries.");
        }
        let coalescingTransactionId = typeof callback === "function"
            ? 0 : transactionId || 0;
        let body = [String(scope)];
        if (scope === "topology") body.push(String(instanceId));
        body.push(String(coalescingTransactionId), entries.length);
        entries.forEach((entry) => {
            body.push("entry");
            body = body.concat(this.encodePath(entry.path));
            body.push("value", this.encodeValue(entry.path, entry.value));
        }, this);
        return this.protocol.request("write", body, callback);
    }
    
    reset(path, callback, transactionId, scope)
    {
        let frame = [String(transactionId || 0), scope || this.scope.mode]
            .concat(this.encodePath(path));
        return this.protocol.request(
            "reset",
            frame,
            callback
        );
    }
    
    encodePath(path)
    {
        let parts = Array.isArray(path) ? path : path.split(".");
        return parts.map((part) => {
            return /^\d+$/.test(String(part)) ? parseInt(part, 10) : part;
        });
    }
    
    encodeValue(path, value)
    {
        let text = Array.isArray(path) ? path.join(".") : path;
        if (/(^|\.)bank\.[0-6]\.group$/.test(text) && value === null) {
            return "none";
        }
        return value;
    }
    
    decodeValue(path, value)
    {
        if (value === "none") return null;
        return value;
    }
    
    decodeOptional(value)
    {
        return value === "none" ? undefined : value;
    }
    
    handleResponseBegin(args)
    {
        if (args.length !== 6) return;
        let requestId = String(args[2]);
        this.responses[requestId] = {
            instanceId: String(args[3]),
            truncated: Number(args[4]) === 1,
            expectedCount: Number(args[5]),
            entries: [],
            invalid: false
        };
    }
    
    handleResponseEntry(args)
    {
        let response = this.responses[String(args[2])];
        if (!response || args.length < 12) return;
        if (Number(args[4]) !== response.entries.length) {
            response.invalid = true;
            return;
        }
        let path = args.slice(5, -6).join(".");
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
    }
    
    handleResponseDone(args)
    {
        if (args.length !== 4) return;
        let requestId = String(args[2]);
        let response = this.responses[requestId];
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
    }
    
    destroy()
    {
        this.protocol = null;
        this.scope = null;
        this.responses = {};
    }
}

module.exports = {
    StateClient: StateClient
};
