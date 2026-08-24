var PROTOCOL_VERSION = 1;

function NativeProtocolClient(source, send) {
    this.source = source;
    this.send = send || function () {};
    this.nextRequestId = 1;
    this.pending = {};
    this.handlers = {};
}

NativeProtocolClient.prototype.on = function (selector, handler) {
    var self = this;
    if (!this.handlers[selector]) this.handlers[selector] = [];
    this.handlers[selector].push(handler);
    return function () { self.off(selector, handler); };
};

NativeProtocolClient.prototype.off = function (selector, handler) {
    this.handlers[selector] = (this.handlers[selector] || []).filter(
        function (candidate) { return candidate !== handler; });
};

NativeProtocolClient.prototype.request = function (selector, body, callback) {
    var requestId = String(this.nextRequestId++);
    this.pending[requestId] = callback || function () {};
    this.send([selector, PROTOCOL_VERSION, this.source, requestId]
        .concat(body || []));
    return requestId;
};

NativeProtocolClient.prototype.initialize = function (callback) {
    return this.request("initialize", [], callback);
};

NativeProtocolClient.prototype.dispatch = function (selector, args) {
    (this.handlers[selector] || []).slice().forEach(function (handler) {
        handler(args || []);
    });
};

NativeProtocolClient.prototype.handleControl = function (selector, args) {
    args = args || [];
    if (!args.length || Number(args[0]) !== PROTOCOL_VERSION) return;
    this.dispatch(selector, args);
    if (selector === "initialized") {
        this.complete(String(args[2]), { instanceId: args[3], error: null });
    } else if (selector === "action_done") {
        this.complete(String(args[2]), {
            status: Number(args[3]) === 1 ? "accepted" : "rejected",
            error: Number(args[3]) === 1 ? null : "rejected"
        });
    } else if (selector === "error") {
        this.complete(String(args[2]), { error: args[3] || "protocol_error" });
    }
};

NativeProtocolClient.prototype.complete = function (requestId, response) {
    var callback = this.pending[String(requestId)];
    if (!callback) return;
    delete this.pending[String(requestId)];
    callback(response || {});
};

NativeProtocolClient.prototype.destroy = function () {
    this.pending = {};
    this.handlers = {};
    this.send = function () {};
};
