const { FeaturePresenterSet } = require("../../../Shared/Controllers/FeaturePresenterSet.js");

class OutputController
{
    constructor(viewModel, scope)
    {
        this.presenters = new FeaturePresenterSet(scope);
        this.presenters.addDial("level", viewModel.level, { unit: "dB" });
        this.presenters.addDial("target", viewModel.target, { unit: "dB" });
        this.presenters.addToggle("limiter", viewModel.limiter, "LIMITER");
    }

    destroy()
    {
        this.presenters.destroy();
    }
}

module.exports = {
    OutputController: OutputController
};
