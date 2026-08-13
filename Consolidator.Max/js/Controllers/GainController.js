include("FeaturePresenterSet.js");

function GainController(viewModel) {
    this.presenters = new FeaturePresenterSet();
    this.presenters.addDial("gain", viewModel.gain,
        { decimals: 1, suffix: " dB" });
}

GainController.prototype.destroy = function () {
    this.presenters.destroy();
};
