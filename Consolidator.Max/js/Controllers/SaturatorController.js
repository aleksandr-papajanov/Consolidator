include("AnalyzerController.js");
include("../Presenters/Analyzer/AnalyzerPresenter.js");
include("FeaturePresenterSet.js");

function SaturatorController(viewModel) {
    this.presenters = new FeaturePresenterSet();
    this.presenters.addDial("drive", viewModel.saturator.drive,
        { decimals: 1, suffix: " dB" });
    this.presenters.addDial("gain", viewModel.saturator.gain,
        { decimals: 1, suffix: " dB" });
    this.presenters.addDial("mix", viewModel.saturator.mix,
        { decimals: 1, suffix: "%", scale: 100 });
    this.presenters.addDial("detectorAmount", viewModel.saturator.detectorAmount,
        { decimals: 1, suffix: "x" });
    this.presenters.addButton("bypass", viewModel.saturator.bypass, "BYPASS");
    this.presenters.addButton("solo", viewModel.saturator.solo, "SOLO");
    this.analyzer = new AnalyzerController(new AnalyzerPresenter({
        mode: "detector",
        frequencyRange: { minimum: 20, maximum: 20000 },
        gainRange: { minimum: -12, maximum: 12 },
        spectrumRange: { minimum: -120, maximum: 0 },
        curves: viewModel.analyzer.saturatorDetectorCurves,
        combined: viewModel.analyzer.saturatorDetectorCombined,
        parameters: viewModel.saturator.detectorFilters || []
    }));
}

SaturatorController.prototype.destroy = function () {
    this.analyzer.presenter.destroy();
    this.presenters.destroy();
};
