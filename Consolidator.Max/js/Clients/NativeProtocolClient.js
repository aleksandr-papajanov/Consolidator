const PROTOCOL_VERSION = 1;
class NativeProtocolClient
{
    constructor(source, send)
    {
        this.source = source;
        this.send = send || (() => {});
        this.nextRequestId = 1;
        this.pending = {};
        this.handlers = {};
    }
    
    on(selector, handler)
    {
        if (!this.handlers[selector]) this.handlers[selector] = [];
        this.handlers[selector].push(handler);
        return () => { this.off(selector, handler); };
    }
    
    off(selector, handler)
    {
        this.handlers[selector] = (this.handlers[selector] || []).filter(
            (candidate) => { return candidate !== handler; });
    }
    
    request(selector, body, callback)
    {
        let requestId = String(this.nextRequestId++);
        if (typeof callback === "function") {
            this.pending[requestId] = callback;
        }
        let frame = [selector, PROTOCOL_VERSION, this.source, requestId]
            .concat(body || []);
        this.send(frame);
        return requestId;
    }
    
    initialize(callback)
    {
        return this.request("initialize", [], callback);
    }
    
    dispatch(selector, args)
    {
        (this.handlers[selector] || []).slice().forEach((handler) => {
            handler(args || []);
        });
    }
    
    handleControl(selector, args)
    {
        args = args || [];
        if (!args.length || Number(args[0]) !== PROTOCOL_VERSION) return;
        this.dispatch(selector, args);
        if (selector === "initialized") {
            this.complete(String(args[2]), {
                instanceId: args[3],
                snapshotContext: args[4],
                error: null
            });
        } else if (selector === "action_done") {
            this.complete(String(args[2]), {
                status: Number(args[3]) === 1 ? "accepted" : "rejected",
                error: Number(args[3]) === 1 ? null : "rejected"
            });
        } else if (selector === "error") {
            this.complete(String(args[2]), { error: args[3] || "protocol_error" });
        }
    }
    
    complete(requestId, response)
    {
        let callback = this.pending[String(requestId)];
        if (!callback) return;
        delete this.pending[String(requestId)];
        callback(response || {});
    }
    
    destroy()
    {
        this.pending = {};
        this.handlers = {};
        this.send = () => {};
    }
}

module.exports = {
    NativeProtocolClient: NativeProtocolClient
};
