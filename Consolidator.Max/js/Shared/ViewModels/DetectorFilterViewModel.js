const { StateValueViewModel } = require("./StateValueViewModel.js");

class DetectorFilterViewModel
{
    constructor(state, device, filterId, definition)
    {
        this.state = state;
        this.definition = definition || { type: "bell", parameters: {
            frequency: {}, q: {}, gain: {}
        } };
        let prefix = device + ".detector.filter." + filterId;
        this.path = prefix;
        this.frequency = this.definition.parameters.frequency
            ? new StateValueViewModel(state, prefix + ".frequency") : null;
        this.q = this.definition.parameters.q
            ? new StateValueViewModel(state, prefix + ".q") : null;
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
        return [this.frequency, this.q, this.gain, this.bypass].filter(
            (value) => value !== null);
    }

    setPosition(frequency, gain, transactionId, callback)
    {
        let values = [{ path: this.gain.path, value: gain }];
        if (this.frequency) values.unshift({ path: this.frequency.path, value: frequency });
        this.state.setMany(values, callback, transactionId);
    }

    reset(callback)
    {
        this.state.reset(this.path, callback);
    }

    destroy()
    {
        if (this.frequency) this.frequency.destroy();
        if (this.q) this.q.destroy();
        this.gain.destroy();
        this.bypass.destroy();
    }
}

module.exports = {
    DetectorFilterViewModel: DetectorFilterViewModel
};
