function CompressorViewModel(state) {
    this.threshold = new StateValueViewModel(state, "compressor.threshold");
    this.ratio = new StateValueViewModel(state, "compressor.ratio");
    this.attack = new StateValueViewModel(state, "compressor.attack");
    this.release = new StateValueViewModel(state, "compressor.release");
    this.gain = new StateValueViewModel(state, "compressor.gain");
    this.mix = new StateValueViewModel(state, "compressor.mix");
    this.bypass = new StateValueViewModel(state, "compressor.bypass");
    this.solo = new StateValueViewModel(state, "compressor.solo");
    this.detectorListen = new StateValueViewModel(
        state,
        "compressor.detector.listen"
    );
    this.detectorFilters = [1, 2].map(function (filterId) {
        return new DetectorFilterViewModel(state, "compressor", filterId);
    });
}

CompressorViewModel.prototype.getStateValues = function () {
    return [
        this.threshold,
        this.ratio,
        this.attack,
        this.release,
        this.gain,
        this.mix,
        this.bypass,
        this.solo,
        this.detectorListen
    ].concat(this.detectorFilters.reduce(function (values, filter) {
        return values.concat(filter.getStateValues());
    }, []));
};

CompressorViewModel.prototype.destroy = function () {
    this.threshold.destroy();
    this.ratio.destroy();
    this.attack.destroy();
    this.release.destroy();
    this.gain.destroy();
    this.mix.destroy();
    this.bypass.destroy();
    this.solo.destroy();
    this.detectorListen.destroy();
    this.detectorFilters.forEach(function (filter) { filter.destroy(); });
};
