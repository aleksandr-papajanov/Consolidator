function AnalysisClient(protocol) {
    this.protocol = protocol;
    this.cache = {};
    this.subscribers = {};
    this.currentView = null;
    this.currentViewRevision = null;
    this.registerFrames();
}

var analysisKeys = {
    spectrum_main: "spectrum.main",
    spectrum_reference: "spectrum.reference",
    spectrum_difference: "spectrum.difference",
    eq_combined: "eq.combined",
    eq_all_banks: "eq.allBanks"
};

AnalysisClient.prototype.registerFrames = function () {
    var self = this;
    Object.keys(analysisKeys)
        .forEach(function (selector) {
            self.protocol.on(selector, function (args) {
                self.publish(
                    analysisKeys[selector],
                    { values: args.slice(3) },
                    args[0],
                    args[1],
                    args[2]
                );
            });
        });
    this.protocol.on("eq_filter", function (args) {
        self.publish(
            "eq.filter." + args[3],
            { values: args.slice(4) },
            args[0],
            args[1],
            args[2]
        );
    });
    this.protocol.on("detector_filter", function (args) {
        self.publish(
            "detector." + args[3] + ".filter." + args[4],
            { active: args[5] === 1, values: args.slice(6) },
            args[0], args[1], args[2]
        );
    });
    this.protocol.on("detector_combined", function (args) {
        self.publish(
            "detector." + args[3] + ".combined",
            { active: args[4] === 1, values: args.slice(5) },
            args[0], args[1], args[2]
        );
    });
    this.protocol.on("meter", function (args) {
        self.publish(
            "meter." + args[3],
            { rmsDb: args[4], peakDb: args[5], smoothedDb: args[6] },
            args[0],
            args[1],
            args[2]
        );
    });
    this.protocol.on("compressor_reduction", function (args) {
        self.publish(
            "compressor.reduction",
            { rmsDb: args[3], peakDb: args[4], smoothedDb: args[5] },
            args[0],
            args[1],
            args[2]
        );
    });
    this.protocol.on("saturator_distortion", function (args) {
        self.publish(
            "saturator.distortion",
            { percent: args[3], smoothedPercent: args[4] },
            args[0],
            args[1],
            args[2]
        );
    });
};

AnalysisClient.prototype.view = function (instanceId, bankId) {
    if (this.currentView &&
        this.currentView.instanceId === instanceId &&
        this.currentView.bankId === bankId) {
        return;
    }

    this.currentView = {
        instanceId: instanceId,
        bankId: bankId
    };
    this.currentViewRevision = null;
    this.invalidate();
    this.protocol.sendMessage("analysis_view", [instanceId, bankId]);
};

AnalysisClient.prototype.invalidate = function () {
    var oldCache = this.cache;
    this.cache = {};

    for (var key in oldCache) {
        if (oldCache.hasOwnProperty(key)) {
            this.notify(key, null);
        }
    }
};

AnalysisClient.prototype.tick = function () {
    if (!this.currentView) {
        return;
    }
    this.protocol.sendMessage("analysis_tick");
};

AnalysisClient.prototype.get = function (key) {
    return this.cache[key];
};

AnalysisClient.prototype.subscribe = function (key, callback, immediate) {
    var self = this;
    if (!this.subscribers[key]) {
        this.subscribers[key] = [];
    }
    this.subscribers[key].push(callback);

    if (immediate && this.cache.hasOwnProperty(key)) {
        callback(this.cache[key]);
    }

    return function () {
        self.unsubscribe(key, callback);
    };
};

AnalysisClient.prototype.unsubscribe = function (key, callback) {
    var listeners = this.subscribers[key] || [];
    this.subscribers[key] = listeners.filter(function (listener) {
        return listener !== callback;
    });
};

AnalysisClient.prototype.publish = function (
    key,
    value,
    viewRevision,
    instanceId,
    bankId
) {
    if (!this.acceptsView(instanceId, bankId)) {
        return;
    }
    var numericViewRevision = Number(viewRevision);
    if (!isFinite(numericViewRevision)) {
        return;
    }
    if (this.currentViewRevision !== null &&
        numericViewRevision < this.currentViewRevision) {
        return;
    }
    if (this.currentViewRevision !== null &&
        numericViewRevision > this.currentViewRevision) {
        this.invalidate();
    }
    this.currentViewRevision = numericViewRevision;
    value.viewRevision = viewRevision;
    value.view = {
        instanceId: instanceId,
        bankId: bankId
    };
    this.cache[key] = value;
    this.notify(key, value);
};

AnalysisClient.prototype.acceptsView = function (instanceId, bankId) {
    return this.currentView !== null &&
        String(this.currentView.instanceId) === String(instanceId) &&
        String(this.currentView.bankId) === String(bankId);
};

AnalysisClient.prototype.notify = function (key, value) {
    var listeners = this.subscribers[key] || [];
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](value);
    }
};
