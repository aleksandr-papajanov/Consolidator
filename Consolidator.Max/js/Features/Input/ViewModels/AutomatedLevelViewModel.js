class AutomatedLevelViewModel
{
    constructor(level, leveler)
    {
        this.level = level;
        this.leveler = leveler;
        this.listeners = [];
        this.unsubscribers = [
            level.subscribe(() =>
            {
                this.notify();
            }, false),
            leveler.subscribe(() =>
            {
                this.notify();
            }, false)
        ];
    }

    get value() { return this.level.value; }
    get minimum() { return this.level.minimum; }
    get maximum() { return this.level.maximum; }
    get physicalMinimum() { return this.level.physicalMinimum; }
    get physicalMaximum() { return this.level.physicalMaximum; }
    get status() { return this.level.status; }
    get instanceId() { return this.level.instanceId; }
    get loaded() { return this.level.loaded; }
    get loading() { return this.level.loading; }
    get enabled() { return this.level.enabled && !Boolean(this.leveler.value); }

    set(value, callback, transactionId)
    {
        if (this.enabled)
        {
            this.level.set(value, callback, transactionId);
        }
    }

    reset(transactionId)
    {
        if (this.enabled)
        {
            this.level.reset(transactionId);
        }
    }

    subscribe(callback, immediate)
    {
        this.listeners.push(callback);
        if (immediate && this.loaded)
        {
            callback(this);
        }

        return () =>
        {
            this.listeners = this.listeners.filter((listener) => listener !== callback);
        };
    }

    notify()
    {
        this.listeners.slice().forEach((listener) =>
        {
            listener(this);
        });
    }

    destroy()
    {
        this.unsubscribers.forEach((unsubscribe) =>
        {
            unsubscribe();
        });
        this.unsubscribers = [];
        this.listeners = [];
    }
}

module.exports = {
    AutomatedLevelViewModel: AutomatedLevelViewModel
};
