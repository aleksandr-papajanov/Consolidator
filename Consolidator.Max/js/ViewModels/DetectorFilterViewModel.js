const { StateValueViewModel } = require("./StateValueViewModel.js");

class DetectorFilterViewModel
{
    constructor(state, device, filterId)
    {
        this.state = state;
        let prefix = device + ".detector.filter." + filterId;
        this.path = prefix;
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
    
    setPosition(frequency, gain, transactionId, callback)
    {
        this.state.setMany([
            { path: this.frequency.path, value: frequency },
            { path: this.gain.path, value: gain }
        ], callback, transactionId);
    }

    reset(callback)
    {
        this.state.reset(this.path, callback);
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
