const { FeaturePresenterSet } = require("../../../Shared/Controllers/FeaturePresenterSet.js");

class PolishController
{
    constructor(viewModel, scope)
    {
        this.presenters = new FeaturePresenterSet(scope);
        this.presenters.addDial("thick", viewModel.polish.thick);
        this.presenters.addDial("air", viewModel.polish.air);
    }

    destroy()
    {
        this.presenters.destroy();
    }
}

module.exports = { PolishController: PolishController };
