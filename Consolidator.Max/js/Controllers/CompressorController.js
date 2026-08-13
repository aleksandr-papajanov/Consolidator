include("AnalyzerController.js");
include("../Presenters/Analyzer/AnalyzerPresenter.js");

function CompressorController(viewModel) {
    this.analyzer = new AnalyzerController(new AnalyzerPresenter({
        mode: "detector",
        frequencyRange: { minimum: 20, maximum: 20000 },
        gainRange: { minimum: -12, maximum: 12 },
        spectrumRange: { minimum: -120, maximum: 0 },
        curves: viewModel.analyzer.compressorDetectorCurves,
        combined: viewModel.analyzer.compressorDetectorCombined,
        parameters: viewModel.compressor.detectorFilters || []
    }));
}

CompressorController.prototype.destroy = function () {
    this.analyzer.presenter.destroy();
};
