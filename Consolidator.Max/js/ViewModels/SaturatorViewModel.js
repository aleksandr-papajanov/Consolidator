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
    ];
};

SaturatorViewModel.prototype.destroy = function () {
    this.drive.destroy();
    this.mix.destroy();
    this.gain.destroy();
    this.detectorAmount.destroy();
    this.bypass.destroy();
    this.solo.destroy();
    this.detectorListen.destroy();
};
