const { InputViewModel } = require("../../Features/Input/ViewModels/InputViewModel.js");
const { OutputViewModel } = require("../../Features/Output/ViewModels/OutputViewModel.js");
const { SaturatorViewModel } = require(
    "../../Features/Saturator/ViewModels/SaturatorViewModel.js");
const { CompressorViewModel } = require(
    "../../Features/Compressor/ViewModels/CompressorViewModel.js");
const { EqualizerViewModel } = require(
    "../../Features/Equalizer/ViewModels/EqualizerViewModel.js");
const { PolishViewModel } = require("../../Features/Polish/ViewModels/PolishViewModel.js");

class ConsolidatorViewModel
{
    constructor(uiTarget)
    {
        this.uiTarget = uiTarget;
        this.targetState = uiTarget.targetState;
        this.input = new InputViewModel(this.targetState);
        this.saturator = new SaturatorViewModel(this.targetState);
        this.compressor = new CompressorViewModel(this.targetState);
        this.equalizer = new EqualizerViewModel(this.targetState);
        this.polish = new PolishViewModel(this.targetState);
        this.output = new OutputViewModel(this.targetState);
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
        this.input.destroy();
        this.saturator.destroy();
        this.compressor.destroy();
        this.equalizer.destroy();
        this.polish.destroy();
        this.output.destroy();
    }
}


module.exports = {
    ConsolidatorViewModel: ConsolidatorViewModel
};