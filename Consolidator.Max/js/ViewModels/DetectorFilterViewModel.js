const { StateValueViewModel } = require("./StateValueViewModel.js");

class DetectorFilterViewModel
{
    constructor(state, device, filterId)
    {
        this.state = state;
        let prefix = device + ".detector.filter." + filterId;
        this.frequency = new StateValueViewModel(state, prefix + ".frequency");
        this.q = new StateValueViewModel(state, prefix + ".q");
        this.gain = new StateValueViewModel(state, prefix + ".gain");
        this.bypass = new StateValueViewModel(state, prefix + ".bypass");
        this.enabled = {
            source: this.bypass,
            read: (value) => { return !value; },
            write: (value) => { return !value; }
        };
    }
    
    getStateValues()
    {
        return [this.frequency, this.q, this.gain, this.bypass];
    }
    
    setPosition(frequency, gain, transactionId)
    {
        this.state.setMany([
            { path: this.frequency.path, value: frequency },
            { path: this.gain.path, value: gain }
        ], undefined, transactionId);
    }
    
    destroy()
    {
        this.frequency.destroy();
        this.q.destroy();
        this.gain.destroy();
        this.bypass.destroy();
    }
}


module.exports = {
    DetectorFilterViewModel: DetectorFilterViewModel
};
