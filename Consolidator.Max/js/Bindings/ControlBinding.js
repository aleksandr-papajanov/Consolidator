function ControlBinding(presenter, sendMessage) {
    this.presenter = presenter;
    this.sendMessage = sendMessage;
    this.unsubscribers = [];
    this.destroyed = false;
}

ControlBinding.prototype.connectPresentation = function () {
    var self = this;
    if (!this.presenter || typeof this.presenter.subscribe !== "function") {
        return;
    }
    this.unsubscribers.push(this.presenter.subscribe(function (presentation) {
        self.applyPresentation(presentation);
    }, true));
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
    this.presenter = null;
    this.sendMessage = null;
};
