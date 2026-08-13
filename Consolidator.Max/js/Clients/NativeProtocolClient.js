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
    if (!this.handlers[selector]) {
        this.handlers[selector] = [];
    }
    this.handlers[selector].push(handler);

    return function () {
        self.off(selector, handler);
    };
};

NativeProtocolClient.prototype.off = function (selector, handler) {
    var handlers = this.handlers[selector] || [];
    this.handlers[selector] = handlers.filter(function (candidate) {
        return candidate !== handler;
    });
};

NativeProtocolClient.prototype.request = function (selector, body, callback) {
    var requestId = String(this.nextRequestId++);
    this.pending[requestId] = callback || function () {};
    this.send([
        selector,
        PROTOCOL_VERSION,
        this.source,
        requestId
    ].concat(body || []));
    return requestId;
};

NativeProtocolClient.prototype.sendMessage = function (selector, args) {
    this.send([selector].concat(args || []));
};

NativeProtocolClient.prototype.dispatch = function (selector, args) {
    var handlers = this.handlers[selector] || [];
    for (var index = 0; index < handlers.length; index += 1) {
        handlers[index](args || []);
    }
};

NativeProtocolClient.prototype.handleControl = function (selector, args) {
    args = args || [];
    if (selector === "registry_changed") {
        if (args.length !== 2 || args[0] !== PROTOCOL_VERSION) {
            return;
        }
        this.dispatch(selector, args);
        return;
    }
    if (args.length < 2) {
        return;
    }
    if (args[0] !== PROTOCOL_VERSION) {
        return;
    }
    if (String(args[1]) !== String(this.source)) {
        return;
    }
    this.dispatch(selector, args);
    if (selector === "error" && args.length >= 6) {
        this.complete(String(args[2]), {
            error: args[4],
            message: args[5]
        });
    }
};

NativeProtocolClient.prototype.handleAnalysis = function (selector, args) {
    this.dispatch(selector, args);
};

NativeProtocolClient.prototype.complete = function (requestId, response) {
    var callback = this.pending[String(requestId)];
    if (!callback) {
        return;
    }
    delete this.pending[String(requestId)];
    callback(response);
};

NativeProtocolClient.prototype.destroy = function () {
    this.pending = {};
    this.handlers = {};
    this.send = function () {};
};
