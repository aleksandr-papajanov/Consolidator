const { AnalyzerController } = require("../../Analyzer/Controllers/AnalyzerController.js");
const { AnalyzerPresenter } = require("../../Analyzer/Presenters/AnalyzerPresenter.js");
const { FeaturePresenterSet } = require("../../../Shared/Controllers/FeaturePresenterSet.js");

class CompressorController
{
    constructor(viewModel, scope)
    {
        this.presenters = new FeaturePresenterSet(scope);
        this.presenters.addDial("attack", viewModel.compressor.attack);
        this.presenters.addDial("sustain", viewModel.compressor.sustain);
        this.presenters.addDial("compression", viewModel.compressor.compression);
        this.presenters.addMultiValueToggle("character", viewModel.compressor.character,
            ["PUNCH", "TIGHT", "SMOOTH"]);
        this.presenters.addToggle("parallel", viewModel.compressor.parallel, "PARALLEL");
        this.presenters.addDial("output", viewModel.compressor.output);
        this.analyzer = new AnalyzerController(new AnalyzerPresenter({
            mode: "detector",
            context: "compressor",
            frequencyRange: { minimum: 20, maximum: 20000 },
            gainRange: { minimum: -24, maximum: 24 },
            statusSource: viewModel.targetState,
            scope: scope,
            parameters: viewModel.compressor.detectorFilters
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
    CompressorController: CompressorController
};
