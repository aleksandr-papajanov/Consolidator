function ObservableValue(initialValue) {
    this.value = initialValue === undefined ? null : initialValue;
    this.listeners = [];
}

ObservableValue.prototype.set = function (value) {
    if (this.value === value) {
        return;
    }
    this.value = value;
    this.notify();
};

ObservableValue.prototype.subscribe = function (callback, immediate) {
    var self = this;
    this.listeners.push(callback);

    if (immediate) {
        callback(this.value);
    }

    return function () {
        self.listeners = self.listeners.filter(function (listener) {
            return listener !== callback;
        });
    };
};

ObservableValue.prototype.notify = function () {
    for (var index = 0; index < this.listeners.length; index += 1) {
        this.listeners[index](this.value);
    }
};
