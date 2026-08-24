function GainViewModel(state, path) {
    this.gain = new StateValueViewModel(state, path + ".gain");
}

GainViewModel.prototype.getStateValues = function () {
    return [this.gain];
};

GainViewModel.prototype.destroy = function () {
    this.gain.destroy();
};
