function ConsolidatorViewModel(uiTarget) {
    this.uiTarget = uiTarget;
    this.targetState = uiTarget.targetState;
    this.inputGain = new GainViewModel(this.targetState, "input_gain");
    this.saturator = new SaturatorViewModel(this.targetState);
    this.compressor = new CompressorViewModel(this.targetState);
    this.equalizer = new EqualizerViewModel(this.targetState);
    this.outputGain = new GainViewModel(this.targetState, "output_gain");
    this.initialized = false;
}

ConsolidatorViewModel.prototype.initialize = function (callback) {
    this.initialized = true;
    if (callback) callback(null);
};

ConsolidatorViewModel.prototype.show = function (instanceId, bankId) {
    return this.uiTarget.show(instanceId, bankId);
};

ConsolidatorViewModel.prototype.destroy = function () {
    this.inputGain.destroy();
    this.saturator.destroy();
    this.compressor.destroy();
    this.equalizer.destroy();
    this.outputGain.destroy();
};
