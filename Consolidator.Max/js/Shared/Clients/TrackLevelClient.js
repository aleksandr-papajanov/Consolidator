class TrackLevelClient
{
    constructor(protocol)
    {
        this.levels = {};
        this.revision = 0;
        this.listeners = [];
        this.unsubscribe = protocol.on("track_levels", (args) => {
            this.handle(args);
        });
    }

    handle(args)
    {
        if (!args || args.length < 3)
        {
            return;
        }
        const revision = Number(args[1]);
        const count = Number(args[2]);
        if (!isFinite(revision) || revision <= this.revision ||
                !isFinite(count) || count < 0 || Math.floor(count) !== count ||
                args.length !== 3 + count * 2)
        {
            return;
        }

        const entries = [];
        for (let index = 0; index < count; index += 1)
        {
            const instanceId = String(args[3 + index * 2]);
            const levelDb = Number(args[4 + index * 2]);
            if (!isFinite(levelDb))
            {
                return;
            }
            entries.push({ instanceId: instanceId, levelDb: levelDb });
        }

        const changedInstanceIds = [];
        entries.forEach((entry) =>
        {
            this.levels[entry.instanceId] = entry.levelDb;
            changedInstanceIds.push(entry.instanceId);
        });
        this.revision = revision;
        this.listeners.slice().forEach((listener) =>
        {
            listener(changedInstanceIds);
        });
    }

    get(instanceId)
    {
        const level = this.levels[String(instanceId)];
        return level === undefined ? -120 : level;
    }

    subscribe(callback)
    {
        this.listeners.push(callback);
        return () =>
        {
            this.listeners = this.listeners.filter((listener) =>
            {
                return listener !== callback;
            });
        };
    }

    destroy()
    {
        if (this.unsubscribe)
        {
            this.unsubscribe();
        }
        this.unsubscribe = null;
        this.listeners = [];
        this.levels = {};
    }
}

module.exports = {
    TrackLevelClient: TrackLevelClient
};
