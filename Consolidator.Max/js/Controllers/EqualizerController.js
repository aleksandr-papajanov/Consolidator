const { AnalyzerController } = require("./AnalyzerController.js");
const { AnalyzerPresenter } = require("../Presenters/Analyzer/AnalyzerPresenter.js");
const { FeaturePresenterSet } = require("./FeaturePresenterSet.js");

class EqualizerController
{
    constructor(viewModel)
    {
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
    }
    
    createBankParameters(filters)
    {
        return filters.map((filter) => {
                return {
                    frequency: filter.frequency,
                    gain: filter.gain,
                    q: filter.q,
                    enabled: filter.bypass ? {
                        source: filter.bypass,
                        read: (value) => { return !value; },
                        write: (value) => { return !value; }
                    } : undefined,
                    setPosition: (frequency, gain, transactionId) => {
                        filter.setPosition(frequency, gain, transactionId);
                    }
                };
            });
    }
    
    destroy()
    {
        this.analyzer.presenter.destroy();
        this.analyzer.presenter = null;
        this.analyzer = null;
        this.presenters.destroy();
    }
}

module.exports = {
    EqualizerController: EqualizerController
};

