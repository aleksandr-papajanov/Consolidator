function StateValueViewModel(state, path) {
    this.state = state;
    this.path = path;
    this.value = undefined;
    this.minimum = undefined;
    this.maximum = undefined;
    this.physicalMinimum = undefined;
    this.physicalMaximum = undefined;
    this.status = undefined;
    this.loaded = false;
    this.listeners = [];

    var self = this;
    this.unsubscribeState = state.subscribe(path, function (entry) {
        self.applyEntry(entry);
    }, true);
}

StateValueViewModel.prototype.applyEntry = function (entry) {
    if (!entry) {
        return;
    }
    this.loaded = true;
    this.value = entry.value;
    this.minimum = entry.min;
    this.maximum = entry.max;
    this.physicalMinimum = entry.physicalMin;
    this.physicalMaximum = entry.physicalMax;
    this.status = entry.status;
    this.notify();
};

StateValueViewModel.prototype.set = function (value) {
    this.state.set(this.path, value);
};

StateValueViewModel.prototype.fetch = function (callback) {
    return this.state.fetch(this.path, callback);
};

StateValueViewModel.prototype.reset = function () {
    this.state.reset(this.path);
};

StateValueViewModel.prototype.subscribe = function (callback, immediate) {
    var self = this;
    this.listeners.push(callback);

    if (immediate && this.loaded) {
        callback(this);
    }

    return function () {
        self.listeners = self.listeners.filter(function (listener) {
            return listener !== callback;
        });
    };
};

StateValueViewModel.prototype.notify = function () {
    for (var index = 0; index < this.listeners.length; index += 1) {
        this.listeners[index](this);
    }
};

StateValueViewModel.prototype.destroy = function () {
    if (this.unsubscribeState) {
        this.unsubscribeState();
        this.unsubscribeState = null;
    }
    this.listeners = [];
};
