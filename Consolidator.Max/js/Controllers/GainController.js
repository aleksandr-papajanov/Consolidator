const { FeaturePresenterSet } = require("./FeaturePresenterSet.js");
const { AnalyzerController } = require("./AnalyzerController.js");
const { AnalyzerPresenter } = require("../Presenters/Analyzer/AnalyzerPresenter.js");

class GainController
{
    constructor(viewModel, scope)
    {
        this.presenters = new FeaturePresenterSet(scope);
        this.presenters.addDial("level", viewModel.level);
        this.presenters.addDial("target", viewModel.target);
        let toggle = viewModel.leveler || viewModel.limiter;
        if (toggle) {
            this.presenters.addToggle(
                viewModel.leveler ? "leveler" : "limiter",
                toggle,
                viewModel.leveler ? "LEVELER" : "LIMITER");
        }
        if (viewModel.width) {
            this.presenters.addDial("width", viewModel.width);
        }
        this.analyzer = viewModel.detectorFilters.length > 0
            ? new AnalyzerController(new AnalyzerPresenter({
                mode: "detector",
                context: "input_gain",
                frequencyRange: { minimum: 20, maximum: 20000 },
                gainRange: { minimum: -24, maximum: 24 },
                statusSource: viewModel.targetState,
                scope: scope,
                parameters: viewModel.detectorFilters
            })) : null;
    }
    
    destroy()
    {
        this.presenters.destroy();
        if (this.analyzer) {
            this.analyzer.presenter.destroy();
            this.analyzer.presenter = null;
            this.analyzer = null;
        }
    }
}

module.exports = {
    GainController: GainController
};
