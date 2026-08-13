function SaturatorViewModel(state) {
    this.drive = new StateValueViewModel(state, "saturator.drive");
    this.mix = new StateValueViewModel(state, "saturator.mix");
    this.gain = new StateValueViewModel(state, "saturator.gain");
    this.detectorAmount = new StateValueViewModel(
        state,
        "saturator.detector_amount"
    );
    this.bypass = new StateValueViewModel(state, "saturator.bypass");
    this.solo = new StateValueViewModel(state, "saturator.solo");
    this.detectorListen = new StateValueViewModel(
        state,
        "saturator.detector.listen"
    );
    this.detectorFilters = [1, 2].map(function (filterId) {
        return new DetectorFilterViewModel(state, "saturator", filterId);
    });
}

SaturatorViewModel.prototype.getStateValues = function () {
    return [
        this.drive,
        this.mix,
        this.gain,
        this.detectorAmount,
        this.bypass,
        this.solo,
        this.detectorListen
    ].concat(this.detectorFilters.reduce(function (values, filter) {
        return values.concat(filter.getStateValues());
    }, []));
};

SaturatorViewModel.prototype.destroy = function () {
    this.drive.destroy();
    this.mix.destroy();
    this.gain.destroy();
    this.detectorAmount.destroy();
    this.bypass.destroy();
    this.solo.destroy();
    this.detectorListen.destroy();
    this.detectorFilters.forEach(function (filter) { filter.destroy(); });
};
