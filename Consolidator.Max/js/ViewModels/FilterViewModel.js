function FilterViewModel(state, bankId, filterId) {
    var prefix = "equalizer.bank." + bankId + ".filter." + filterId;
    this.bankId = bankId;
    this.filterId = filterId;
    this.frequency = new StateValueViewModel(state, prefix + ".frequency");
    this.q = new StateValueViewModel(state, prefix + ".q");
    this.gain = new StateValueViewModel(state, prefix + ".gain");
    this.bypass = new StateValueViewModel(state, prefix + ".bypass");
    this.solo = new StateValueViewModel(state, prefix + ".solo");
}

FilterViewModel.prototype.getStateValues = function () {
    return [this.frequency, this.q, this.gain, this.bypass, this.solo];
};

FilterViewModel.prototype.destroy = function () {
    this.frequency.destroy();
    this.q.destroy();
    this.gain.destroy();
    this.bypass.destroy();
    this.solo.destroy();
};
