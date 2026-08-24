include("Project:/js/Controllers/AnalyzerController.js");
include("Project:/js/Presenters/Analyzer/AnalyzerPresenter.js");
include("Project:/js/Controllers/FeaturePresenterSet.js");

function CompressorController(viewModel) {
    this.presenters = new FeaturePresenterSet();
    this.presenters.addDial("threshold", viewModel.compressor.threshold,
        { decimals: 1, suffix: " dB" });
    this.presenters.addDial("ratio", viewModel.compressor.ratio,
        { decimals: 1, suffix: ":1" });
    this.presenters.addDial("attack", viewModel.compressor.attack,
        { decimals: 1, suffix: " ms" });
    this.presenters.addDial("release", viewModel.compressor.release,
        { decimals: 1, suffix: " ms" });
    this.presenters.addDial("gain", viewModel.compressor.gain,
        { decimals: 1, suffix: " dB" });
    this.presenters.addDial("mix", viewModel.compressor.mix,
        { decimals: 1, suffix: "%", scale: 100 });
    this.presenters.addButton("bypass", viewModel.compressor.bypass, "BYPASS");
    this.presenters.addButton("solo", viewModel.compressor.solo, "SOLO");
    this.analyzer = new AnalyzerController(new AnalyzerPresenter({
        mode: "detector",
        frequencyRange: { minimum: 20, maximum: 20000 },
        gainRange: { minimum: -12, maximum: 12 },
        statusSource: viewModel.targetState,
        parameters: viewModel.compressor.detectorFilters
    }));
}

CompressorController.prototype.destroy = function () {
    this.analyzer.presenter.destroy();
    this.analyzer.presenter = null;
    this.analyzer = null;
    this.presenters.destroy();
};
