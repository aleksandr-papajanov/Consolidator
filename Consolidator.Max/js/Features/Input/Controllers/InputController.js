const { FeaturePresenterSet } = require("../../../Shared/Controllers/FeaturePresenterSet.js");
const { AnalyzerController } = require("../../Analyzer/Controllers/AnalyzerController.js");
const { AnalyzerPresenter } = require("../../Analyzer/Presenters/AnalyzerPresenter.js");

class InputController
{
    constructor(viewModel, scope)
    {
        this.presenters = new FeaturePresenterSet(scope);
        this.presenters.addDial("level", viewModel.level);
        this.presenters.addDial("target", viewModel.target);
        this.presenters.addToggle("leveler", viewModel.leveler, "LEVELER");
        this.presenters.addDial("width", viewModel.width);
        this.analyzer = new AnalyzerController(new AnalyzerPresenter({
            mode: "detector",
            context: "input_gain",
            frequencyRange: { minimum: 20, maximum: 20000 },
            gainRange: { minimum: -24, maximum: 24 },
            statusSource: viewModel.targetState,
            scope: scope,
            parameters: viewModel.detectorFilters
        }));
    }

    destroy()
    {
        this.presenters.destroy();
        this.analyzer.presenter.destroy();
        this.analyzer.presenter = null;
        this.analyzer = null;
    }
}

module.exports = {
    InputController: InputController
};
