const { GainViewModel } = require("./GainViewModel.js");
const { SaturatorViewModel } = require("./SaturatorViewModel.js");
const { CompressorViewModel } = require("./CompressorViewModel.js");
const { EqualizerViewModel } = require("./EqualizerViewModel.js");
const { PolishViewModel } = require("./PolishViewModel.js");

class ConsolidatorViewModel
{
    constructor(uiTarget)
    {
        this.uiTarget = uiTarget;
        this.targetState = uiTarget.targetState;
        this.inputGain = new GainViewModel(this.targetState, "input_gain");
        this.saturator = new SaturatorViewModel(this.targetState);
        this.compressor = new CompressorViewModel(this.targetState);
        this.equalizer = new EqualizerViewModel(this.targetState);
        this.polish = new PolishViewModel(this.targetState);
        this.outputGain = new GainViewModel(this.targetState, "output_gain");
        this.initialized = false;
    }
    
    initialize(callback)
    {
        this.initialized = true;
        if (callback) callback(null);
    }
    
    show(instanceId, bankId, snapshotContext, callback)
    {
        return this.uiTarget.show(instanceId, bankId, snapshotContext, callback);
    }
    
    destroy()
    {
        this.inputGain.destroy();
        this.saturator.destroy();
        this.compressor.destroy();
        this.equalizer.destroy();
        this.polish.destroy();
        this.outputGain.destroy();
    }
}


module.exports = {
    ConsolidatorViewModel: ConsolidatorViewModel
};
