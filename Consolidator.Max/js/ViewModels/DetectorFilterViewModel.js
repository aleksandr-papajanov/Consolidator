function DetectorFilterViewModel(state, device, filterId) {
    this.state = state;
    var prefix = device + ".detector.filter." + filterId;
    this.frequency = new StateValueViewModel(state, prefix + ".frequency");
    this.q = new StateValueViewModel(state, prefix + ".q");
    this.gain = new StateValueViewModel(state, prefix + ".gain");
    this.bypass = new StateValueViewModel(state, prefix + ".bypass");
    this.enabled = {
        source: this.bypass,
        read: function (value) { return !value; },
        write: function (value) { return !value; }
    };
}

DetectorFilterViewModel.prototype.getStateValues = function () {
    return [this.frequency, this.q, this.gain, this.bypass];
};

DetectorFilterViewModel.prototype.setPosition = function (frequency, gain) {
    this.state.setMany([
        { path: this.frequency.path, value: frequency },
        { path: this.gain.path, value: gain }
    ]);
};

DetectorFilterViewModel.prototype.destroy = function () {
    this.frequency.destroy();
    this.q.destroy();
    this.gain.destroy();
    this.bypass.destroy();
};
