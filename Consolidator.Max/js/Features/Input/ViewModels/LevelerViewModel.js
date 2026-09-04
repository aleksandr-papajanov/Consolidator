class LevelerViewModel
{
    constructor(state, source)
    {
        this.state = state;
        this.leveler = source;
    }

    get value() { return this.leveler.value; }
    get loaded() { return this.leveler.loaded; }
    get loading() { return this.leveler.loading; }
    get enabled() { return this.leveler.enabled; }

    set(value, callback, transactionId)
    {
        if (Boolean(value))
        {
            this.state.setMany([
                { path: "input_gain.leveler", value: true }
            ], callback, transactionId);
            return;
        }

        this.state.disableInputLeveler(callback);
    }

    reset(transactionId)
    {
        this.set(false, undefined, transactionId);
    }

    subscribe(callback, immediate)
    {
        return this.leveler.subscribe(callback, immediate);
    }

    destroy()
    {
        this.state = null;
        this.leveler = null;
    }
}

module.exports = {
    LevelerViewModel: LevelerViewModel
};
