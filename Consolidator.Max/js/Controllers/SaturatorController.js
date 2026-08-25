include("Project:/js/Controllers/AnalyzerController.js");
include("Project:/js/Presenters/Analyzer/AnalyzerPresenter.js");
include("Project:/js/Controllers/FeaturePresenterSet.js");

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
        gainRange: { minimum: -24, maximum: 24 },
        statusSource: viewModel.targetState,
        parameters: viewModel.saturator.detectorFilters
    }));
}

SaturatorController.prototype.destroy = function () {
    this.analyzer.presenter.destroy();
    this.analyzer.presenter = null;
    this.analyzer = null;
    this.presenters.destroy();
};
