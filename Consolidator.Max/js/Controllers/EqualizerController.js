include("AnalyzerController.js");
include("../Presenters/Analyzer/AnalyzerPresenter.js");
include("FeaturePresenterSet.js");

function EqualizerController(viewModel) {
    this.viewModel = viewModel;
    this.presenters = new FeaturePresenterSet();
    this.presenters.addButton("bypass", viewModel.equalizer.bypass, "BYPASS");
    this.presenters.addButton("solo", viewModel.equalizer.solo, "SOLO");
    this.analyzerPresenter = new AnalyzerPresenter({
        mode: "equalizer",
        frequencyRange: { minimum: 20, maximum: 20000 },
        gainRange: { minimum: -24, maximum: 24 },
        spectrumRange: { minimum: -120, maximum: 0 },
        spectrum: viewModel.analyzer.mainSpectrum,
        referenceSpectrum: viewModel.analyzer.referenceSpectrum,
        differenceSpectrum: viewModel.analyzer.differenceSpectrum,
        curves: viewModel.analyzer.filterCurves,
        combined: viewModel.analyzer.combinedCurve,
        parameters: []
    });
    this.analyzer = new AnalyzerController(this.analyzerPresenter);
    var self = this;
    this.unsubscribeCurrentBank = viewModel.equalizer.currentBankChanged.subscribe(
        function (bank) { self.bindBank(bank); }, true);
}

EqualizerController.prototype.bindBank = function (bank) {
    this.analyzerPresenter.setParameters(this.createBankParameters(bank));
};

EqualizerController.prototype.createBankParameters = function (bank) {
    return bank ? bank.filters.map(function (filter) {
            return {
                frequency: filter.frequency,
                gain: filter.gain,
                q: filter.q,
                enabled: filter.bypass ? {
                    source: filter.bypass,
                    read: function (value) { return !value; },
                    write: function (value) { return !value; }
                } : undefined,
                setPosition: function (frequency, gain) {
                    filter.setPosition(frequency, gain);
                }
            };
        }) : [];
};

EqualizerController.prototype.destroy = function () {
    if (this.unsubscribeCurrentBank) {
        this.unsubscribeCurrentBank();
        this.unsubscribeCurrentBank = null;
    }
    if (this.analyzer) {
        this.analyzerPresenter.destroy();
        this.analyzer = null;
        this.analyzerPresenter = null;
    }
    this.presenters.destroy();
};
