const { AnalyzerController } = require("../../Analyzer/Controllers/AnalyzerController.js");
const { AnalyzerPresenter } = require("../../Analyzer/Presenters/AnalyzerPresenter.js");
const { FeaturePresenterSet } = require("../../../Shared/Controllers/FeaturePresenterSet.js");

class SaturatorController
{
    constructor(viewModel, scope)
    {
        this.presenters = new FeaturePresenterSet(scope);
        this.presenters.addDial("drive", viewModel.saturator.drive);
        this.presenters.addDial("curve", viewModel.saturator.curve);
        this.presenters.addToggle("split", viewModel.saturator.split, "SPLIT");
        this.presenters.addDial("output", viewModel.saturator.output);
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
