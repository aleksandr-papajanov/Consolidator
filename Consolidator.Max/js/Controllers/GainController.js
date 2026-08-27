const { FeaturePresenterSet } = require("./FeaturePresenterSet.js");

class GainController
{
    constructor(viewModel)
    {
        this.presenters = new FeaturePresenterSet();
        this.presenters.addDial("gain", viewModel.gain,
            { decimals: 1, suffix: " dB" });
    }
    
    destroy()
    {
        this.presenters.destroy();
    }
}

module.exports = {
    GainController: GainController
};

