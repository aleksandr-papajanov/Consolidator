include("Project:/js/Controllers/AnalyzerController.js");
include("Project:/js/Presenters/Analyzer/AnalyzerPresenter.js");
include("Project:/js/Controllers/FeaturePresenterSet.js");

function EqualizerController(viewModel) {
    this.viewModel = viewModel;
    this.presenters = new FeaturePresenterSet();
    this.presenters.addButton("bypass", viewModel.equalizer.bankBypass,
        "BYPASS");
    this.presenters.addButton("solo", viewModel.equalizer.bankSolo, "SOLO");
    this.analyzer = new AnalyzerController(new AnalyzerPresenter({
        mode: "equalizer",
        frequencyRange: { minimum: 20, maximum: 20000 },
        gainRange: { minimum: -24, maximum: 24 },
        statusSource: viewModel.targetState,
        parameters: this.createBankParameters(viewModel.equalizer.filters)
    }));
};

EqualizerController.prototype.createBankParameters = function (filters) {
    return filters.map(function (filter) {
            return {
                frequency: filter.frequency,
                gain: filter.gain,
                q: filter.q,
                enabled: filter.bypass ? {
                    source: filter.bypass,
                    read: function (value) { return !value; },
                    write: function (value) { return !value; }
                } : undefined,
                setPosition: function (frequency, gain, transactionId) {
                    filter.setPosition(frequency, gain, transactionId);
                }
            };
        });
};

EqualizerController.prototype.destroy = function () {
    this.analyzer.presenter.destroy();
    this.analyzer.presenter = null;
    this.analyzer = null;
    this.presenters.destroy();
};
