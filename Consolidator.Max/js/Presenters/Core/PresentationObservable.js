function PresentationObservable() {
    this.listeners = [];
    this.destroyed = false;
}

PresentationObservable.prototype.subscribe = function (callback, immediate) {
    var self = this;
    if (this.destroyed) {
        return function () {};
    }

    this.listeners.push(callback);
    if (immediate && this.presentation) {
        callback(this.presentation);
    }

    return function () {
        self.listeners = self.listeners.filter(function (listener) {
            return listener !== callback;
        });
    };
};

PresentationObservable.prototype.publish = function (presentation) {
    if (this.destroyed) {
        return;
    }

    this.presentation = presentation;
    var listeners = this.listeners.slice();
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](presentation);
    }
};

PresentationObservable.prototype.destroy = function () {
    this.destroyed = true;
    this.listeners = [];
    this.presentation = null;
};
