const { AnalyzerController } = require("./AnalyzerController.js");
const { AnalyzerPresenter } = require("../Presenters/Analyzer/AnalyzerPresenter.js");
const { FeaturePresenterSet } = require("./FeaturePresenterSet.js");

class EqualizerController
{
    constructor(viewModel, scope)
    {
        this.viewModel = viewModel;
        this.presenters = new FeaturePresenterSet(scope);
        this.analyzer = new AnalyzerController(new AnalyzerPresenter({
            mode: "equalizer",
            context: "equalizer",
            frequencyRange: { minimum: 20, maximum: 20000 },
            gainRange: { minimum: -24, maximum: 24 },
            statusSource: viewModel.targetState,
            scope: scope,
            parameters: this.createBankParameters(viewModel.equalizer.filters)
        }));
    }
    
    createBankParameters(filters)
    {
        return filters.map((filter) => {
                return {
                    type: filter.definition.type,
                    definition: filter.definition,
                    frequency: filter.frequency,
                    gain: filter.gain,
                    q: filter.q,
                    enabled: filter.bypass ? {
                        source: filter.bypass,
                        read: (value) => { return !value; },
                        write: (value) => { return !value; }
                    } : undefined,
                    setPosition: (frequency, gain, transactionId, callback) => {
                        filter.setPosition(
                            frequency,
                            gain,
                            transactionId,
                            callback);
                    },
                    reset: (callback) => filter.reset(callback)
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
