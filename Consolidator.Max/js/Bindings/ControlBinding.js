function ControlBinding(presenter, sendMessage) {
    this.presenter = presenter;
    this.sendMessage = sendMessage;
    this.unsubscribers = [];
    this.destroyed = false;
    this.presentationActive = true;
    this.pendingPresentation = null;
}

ControlBinding.prototype.connectPresentation = function () {
    var self = this;
    if (!this.presenter || typeof this.presenter.subscribe !== "function") {
        return;
    }
    this.unsubscribers.push(this.presenter.subscribe(function (presentation) {
        self.receivePresentation(presentation);
    }, true));
};

ControlBinding.prototype.receivePresentation = function (presentation) {
    if (!this.presentationActive) {
        this.pendingPresentation = presentation;
        return;
    }
    this.applyPresentation(presentation);
};

ControlBinding.prototype.setPresentationActive = function (active) {
    var next = Boolean(active);
    if (this.destroyed || this.presentationActive === next) {
        return;
    }
    this.presentationActive = next;
    if (next) {
        this.refreshPresentation();
    }
};

ControlBinding.prototype.refreshPresentation = function () {
    var presentation = this.pendingPresentation ||
        (this.presenter && this.presenter.presentation);
    this.pendingPresentation = null;
    if (presentation) {
        this.applyPresentation(presentation);
    }
};

ControlBinding.prototype.send = function (selector, args) {
    if (typeof this.sendMessage !== "function") {
        return;
    }
    this.sendMessage(selector, args || []);
};

ControlBinding.prototype.handleIntent = function () {
};

ControlBinding.prototype.destroy = function () {
    if (this.destroyed) {
        return;
    }
    this.destroyed = true;
    this.unsubscribers.forEach(function (unsubscribe) { unsubscribe(); });
    this.unsubscribers = [];
    this.pendingPresentation = null;
    this.presenter = null;
    this.sendMessage = null;
};
