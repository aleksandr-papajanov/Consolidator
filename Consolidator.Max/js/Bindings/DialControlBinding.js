include("Project:/js/Bindings/ControlBinding.js");

function DialControlBinding(presenter, sendMessage, transactions) {
    ControlBinding.call(this, presenter, sendMessage);
    this.transactions = transactions;
    this.activeTransactionId = null;
    this.transactionReady = false;
    this.pendingValue = null;
    this.pendingEnd = null;
    this.connectPresentation();
}

DialControlBinding.prototype = Object.create(ControlBinding.prototype);
DialControlBinding.prototype.constructor = DialControlBinding;

DialControlBinding.prototype.applyPresentation = function (presentation) {
    if (this.hasPresentation && !this.requiresFullPresentation(presentation)) {
        this.send("presentation_begin");
        (presentation.rings || []).forEach(function (ring, index) {
            var previous = this.presentation.rings[index];
            if (previous.value !== ring.value) {
                this.send("set", [index, ring.value]);
            }
        }, this);
        this.send("presentation_end");
        this.presentation = presentation;
        return;
    }
    this.send("presentation_begin");
    this.send("enabled", [presentation.enabled ? 1 : 0]);
    this.send("active", [presentation.active ? 1 : 0]);
    this.send("activeIndex", [presentation.activeIndex || 0]);
    this.send("displayIndex", [presentation.displayIndex || 0]);
    this.send("ringCount", [(presentation.rings || []).length]);
    (presentation.rings || []).forEach(function (ring, index) {
        this.send("limits", [index, ring.minimum, ring.maximum]);
        this.send("set", [index, ring.value]);
    }, this);
    this.send("presentation_end");
    this.presentation = presentation;
    this.hasPresentation = true;
};

DialControlBinding.prototype.requiresFullPresentation = function (presentation) {
    var previous = this.presentation || {};
    if (Boolean(previous.enabled) !== Boolean(presentation.enabled) ||
            Boolean(previous.active) !== Boolean(presentation.active) ||
            Number(previous.activeIndex) !== Number(presentation.activeIndex) ||
            Number(previous.displayIndex) !== Number(presentation.displayIndex)) {
        return true;
    }
    var previousRings = previous.rings || [];
    var rings = presentation.rings || [];
    if (previousRings.length !== rings.length) return true;
    for (var index = 0; index < rings.length; index += 1) {
        if (previousRings[index].minimum !== rings[index].minimum ||
                previousRings[index].maximum !== rings[index].maximum ||
                JSON.stringify(previousRings[index].visualization) !==
                    JSON.stringify(rings[index].visualization) ||
                JSON.stringify(previousRings[index].color) !==
                    JSON.stringify(rings[index].color) ||
                JSON.stringify(previousRings[index].display) !==
                    JSON.stringify(rings[index].display)) {
            return true;
        }
    }
    return false;
};

DialControlBinding.prototype.handleIntent = function (name, values) {
    switch (name) {
    case "valueChanged":
        if (this.activeTransactionId !== null && !this.transactionReady) {
            this.pendingValue = [values[0], values[1]];
            break;
        }
        this.presenter.setValue(
            values[0],
            values[1],
            this.activeTransactionId);
        break;
    case "reset":
        this.presenter.resetValue(values[0]);
        break;
    case "gestureBegan":
        if (this.transactions) {
            this.transactionReady = false;
            this.pendingValue = null;
            this.pendingEnd = null;
            this.activeTransactionId = this.transactions.begin(
                this.handleTransactionBegin.bind(this));
        }
        this.presenter.beginGesture(
            values[0],
            this.activeTransactionId);
        break;
    case "gestureEnded":
        if (this.transactions && this.activeTransactionId !== null) {
            if (this.transactionReady) {
                this.presenter.endGesture(values[0], this.activeTransactionId);
                this.finishTransaction();
            }
            else {
                this.pendingEnd = values[0];
            }
        }
        else {
            this.presenter.endGesture(values[0], null);
        }
        break;
    }
};

DialControlBinding.prototype.handleTransactionBegin = function (id, response) {
    if (!this.transactions || this.activeTransactionId !== id) {
        return;
    }
    if (!response || response.status !== "accepted") {
        this.send("transactionRejected");
        this.clearTransaction();
        return;
    }

    this.transactionReady = true;
    if (this.pendingValue) {
        this.presenter.setValue(
            this.pendingValue[0],
            this.pendingValue[1],
            id);
        this.pendingValue = null;
    }
    if (this.pendingEnd !== null) {
        this.presenter.endGesture(this.pendingEnd, id);
        this.finishTransaction();
    }
};

DialControlBinding.prototype.finishTransaction = function () {
    this.transactions.end(this.activeTransactionId);
    this.clearTransaction();
};

DialControlBinding.prototype.clearTransaction = function () {
    this.activeTransactionId = null;
    this.transactionReady = false;
    this.pendingValue = null;
    this.pendingEnd = null;
};

DialControlBinding.prototype.destroy = function () {
    if (this.transactions && this.activeTransactionId !== null &&
            this.transactionReady) {
        this.transactions.end(this.activeTransactionId);
    }
    this.clearTransaction();
    this.transactions = null;
    ControlBinding.prototype.destroy.call(this);
};
