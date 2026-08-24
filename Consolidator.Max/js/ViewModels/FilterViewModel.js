function FilterViewModel(state, filterId) {
    this.state = state;
    var prefix = "equalizer.filter." + filterId;
    this.filterId = filterId;
    this.frequency = new StateValueViewModel(state, prefix + ".frequency");
    this.q = new StateValueViewModel(state, prefix + ".q");
    this.gain = new StateValueViewModel(state, prefix + ".gain");
    this.bypass = new StateValueViewModel(state, prefix + ".bypass");
}

FilterViewModel.prototype.getStateValues = function () {
    return [this.frequency, this.q, this.gain, this.bypass];
};

FilterViewModel.prototype.setPosition = function (frequency, gain, transactionId) {
    this.state.setMany([
        { path: this.frequency.path, value: frequency },
        { path: this.gain.path, value: gain }
    ], undefined, transactionId);
};

FilterViewModel.prototype.destroy = function () {
    this.frequency.destroy();
    this.q.destroy();
    this.gain.destroy();
    this.bypass.destroy();
};
