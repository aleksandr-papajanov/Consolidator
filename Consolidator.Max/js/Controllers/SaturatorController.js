const { AnalyzerController } = require("./AnalyzerController.js");
const { AnalyzerPresenter } = require("../Presenters/Analyzer/AnalyzerPresenter.js");
const { FeaturePresenterSet } = require("./FeaturePresenterSet.js");

class SaturatorController
{
    constructor(viewModel, scope)
    {
        this.presenters = new FeaturePresenterSet(scope);
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
            context: "saturator",
            frequencyRange: { minimum: 20, maximum: 20000 },
            gainRange: { minimum: -24, maximum: 24 },
            statusSource: viewModel.targetState,
            scope: scope,
            parameters: viewModel.saturator.detectorFilters
        }));
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
    SaturatorController: SaturatorController
};
