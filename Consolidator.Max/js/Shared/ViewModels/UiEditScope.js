class UiEditScope
{
    constructor()
    {
        this.mode = "local";
        this.enabled = false;
        this.color = null;
        this.listeners = [];
    }

    isGroup()
    {
        return this.mode === "group" && this.enabled;
    }

    setGroupContext(enabled, color)
    {
        let nextEnabled = Boolean(enabled);
        let nextMode = nextEnabled ? this.mode : "local";
        let changed = this.enabled !== nextEnabled || this.mode !== nextMode ||
            this.color !== color;
        this.enabled = nextEnabled;
        this.mode = nextMode;
        this.color = nextEnabled ? color : null;
        if (changed) this.notify();
    }

    toggle()
    {
        if (!this.enabled) return;
        this.mode = this.isGroup() ? "local" : "group";
        this.notify();
    }

    subscribe(callback)
    {
        this.listeners.push(callback);
        return () => {
            this.listeners = this.listeners.filter((listener) => listener !== callback);
        };
    }

    notify()
    {
        this.listeners.slice(0).forEach((listener) => listener(this));
    }
}

module.exports = {
    UiEditScope: UiEditScope
};
