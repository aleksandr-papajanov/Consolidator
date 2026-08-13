include("AnalyzerController.js");
include("../Presenters/Analyzer/AnalyzerPresenter.js");

function EqualizerController(viewModel) {
    this.viewModel = viewModel;
    this.analyzer = null;
    var self = this;
    this.unsubscribeCurrentBank = viewModel.equalizer.currentBankChanged.subscribe(
        function (bank) { self.bindBank(bank); }, true);
}

EqualizerController.prototype.bindBank = function (bank) {
    if (this.analyzer) {
        this.analyzer.presenter.destroy();
    }
    this.analyzer = new AnalyzerController(new AnalyzerPresenter({
        mode: "equalizer",
        frequencyRange: { minimum: 20, maximum: 20000 },
        gainRange: { minimum: -24, maximum: 24 },
        spectrumRange: { minimum: -120, maximum: 0 },
        spectrum: this.viewModel.analyzer.mainSpectrum,
        referenceSpectrum: this.viewModel.analyzer.referenceSpectrum,
        differenceSpectrum: this.viewModel.analyzer.differenceSpectrum,
        curves: this.viewModel.analyzer.filterCurves,
        combined: this.viewModel.analyzer.combinedCurve,
        parameters: bank ? bank.filters.map(function (filter) {
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
        }) : []
    }));
};

EqualizerController.prototype.destroy = function () {
    if (this.unsubscribeCurrentBank) {
        this.unsubscribeCurrentBank();
        this.unsubscribeCurrentBank = null;
    }
    if (this.analyzer) {
        this.analyzer.presenter.destroy();
        this.analyzer = null;
    }
};
