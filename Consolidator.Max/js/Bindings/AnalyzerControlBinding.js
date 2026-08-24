include("Project:/js/Bindings/ControlBinding.js");

function AnalyzerControlBinding(controller, presenter, sendMessage, transactions) {
    ControlBinding.call(this, presenter, sendMessage);
    this.controller = controller;
    this.transactions = transactions;
    this.activeTransactionId = null;
    this.transactionReady = false;
    this.pendingMove = null;
    this.pendingEnd = false;
    this.connectPresentation();
}

AnalyzerControlBinding.prototype = Object.create(ControlBinding.prototype);
AnalyzerControlBinding.prototype.constructor = AnalyzerControlBinding;

AnalyzerControlBinding.prototype.applyPresentation = function (presentation) {
    this.send("presentation_begin", [
        presentation.mode || "equalizer",
        presentation.enabled ? 1 : 0,
        presentation.parameterRevision || 0,
        presentation.viewKey || ""
    ]);
    this.sendCurve("spectrum", presentation.spectrum);
    this.sendCurve("reference_spectrum", presentation.referenceSpectrum);
    this.sendCurve("difference_spectrum", presentation.differenceSpectrum);
    (presentation.curves || []).forEach(function (curve) {
        this.sendCurve("curve", curve, curve.id);
    }, this);
    this.sendCurve("combined", presentation.combinedCurve);
    this.sendCurve("all_banks", presentation.allBanksCurve);
    (presentation.handles || []).forEach(function (handle) {
        var capabilities = handle.capabilities || {};
        this.send("handle", [
            handle.id,
            handle.frequency,
            handle.gain,
            handle.enabled ? 1 : 0,
            capabilities.frequency ? 1 : 0,
            capabilities.gain ? 1 : 0,
            capabilities.q ? 1 : 0,
            handle.selected ? 1 : 0
        ]);
    }, this);
    this.send("presentation_end");
};

AnalyzerControlBinding.prototype.sendCurve = function (name, curve, id) {
    if (!curve) {
        return;
    }
    var args = id === undefined ? [] : [id];
    args.push(curve.active === false ? 0 : 1);
    args = args.concat(curve.values || []);
    this.send(name, args);
};

AnalyzerControlBinding.prototype.handleIntent = function (name, values) {
    if (name === "gestureBegan") {
        this.beginTransaction();
        return;
    }
    if (name === "filterMoved" && this.activeTransactionId !== null &&
            !this.transactionReady) {
        this.pendingMove = values.slice(0);
        return;
    }
    if (name === "gestureEnded") {
        if (this.activeTransactionId !== null && !this.transactionReady) {
            this.pendingEnd = true;
        }
        else {
            this.finishTransaction();
        }
        return;
    }
    this.controller.handle(name, values, this.activeTransactionId);
};

AnalyzerControlBinding.prototype.beginTransaction = function () {
    if (!this.transactions || this.activeTransactionId !== null) {
        return;
    }
    var self = this;
    this.transactionReady = false;
    this.pendingMove = null;
    this.pendingEnd = false;
    this.activeTransactionId = this.transactions.begin(function (id, response) {
        self.completeTransactionBegin(id, response);
    });
};

AnalyzerControlBinding.prototype.completeTransactionBegin = function (
    id, response
) {
    if (!this.transactions || this.activeTransactionId !== id) {
        return;
    }
    if (!response || response.status !== "accepted") {
        this.send("transactionRejected");
        this.clearTransaction();
        return;
    }
    this.transactionReady = true;
    if (this.pendingMove) {
        this.controller.handle("filterMoved", this.pendingMove, id);
        this.pendingMove = null;
    }
    if (this.pendingEnd) {
        this.finishTransaction();
    }
};

AnalyzerControlBinding.prototype.finishTransaction = function () {
    if (this.transactions && this.activeTransactionId !== null &&
            this.transactionReady) {
        this.transactions.end(this.activeTransactionId);
    }
    this.clearTransaction();
};

AnalyzerControlBinding.prototype.clearTransaction = function () {
    this.activeTransactionId = null;
    this.transactionReady = false;
    this.pendingMove = null;
    this.pendingEnd = false;
};

AnalyzerControlBinding.prototype.destroy = function () {
    this.finishTransaction();
    this.transactions = null;
    this.controller = null;
    ControlBinding.prototype.destroy.call(this);
};
