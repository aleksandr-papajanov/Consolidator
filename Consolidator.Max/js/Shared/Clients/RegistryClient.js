const { RegistryDeltaApplier } = require("./RegistryDeltaApplier.js");
const {
    RegistrySnapshotAssembler,
    processorMarkersChanged
} = require("./RegistrySnapshotAssembler.js");

const REVISIONED_DELTA_SELECTORS = [
    "registry_instance_added",
    "registry_instance_removed",
    "registry_label_changed",
    "registry_instance_mute_changed",
    "registry_instance_solo_changed",
    "registry_instance_bypass_changed",
    "registry_processor_changed",
    "registry_bank_group_changed",
    "registry_bank_effect_changed",
    "registry_bank_bypass_changed"
];

class RegistryClient
{
    constructor(protocol)
    {
        this.protocol = protocol;
        this.snapshot = null;
        this.subscribers = [];
        this.fetchPending = false;
        this.requiredRevision = 0;
        this.assembler = new RegistrySnapshotAssembler();
        this.deltaApplier = new RegistryDeltaApplier();
        this.unsubscribers = [];
        this.connectProtocol();
    }

    get responses()
    {
        return this.assembler.responses;
    }

    connectProtocol()
    {
        const handlers = {
            registry_begin: (args) => this.assembler.begin(args),
            registry_instance: (args) => this.assembler.addInstance(args),
            registry_processor: (args) => this.assembler.addProcessor(args),
            registry_bank: (args) => this.assembler.addBank(args),
            registry_group: (args) => this.assembler.addGroup(args),
            registry_member: (args) => this.assembler.addMember(args),
            registry_done: (args) => this.handleDone(args),
            registry_processor_markers_changed: (args) => {
                this.handleProcessorMarkersChanged(args);
            },
            error: (args) => this.assembler.discard(args[2])
        };
        REVISIONED_DELTA_SELECTORS.forEach((selector) => {
            handlers[selector] = (args) => this.handleDelta(selector, args);
        });
        Object.keys(handlers).forEach((selector) => {
            this.unsubscribers.push(this.protocol.on(selector, handlers[selector]));
        });
    }

    get()
    {
        return this.snapshot;
    }

    fetch(callback)
    {
        this.fetchPending = true;
        return this.protocol.request("registry", [], (response) => {
            this.fetchPending = false;
            if (callback)
            {
                callback(response.error ? undefined : this.snapshot, response);
            }
            if (!response.error &&
                    (!this.snapshot || this.snapshot.revision < this.requiredRevision))
            {
                this.fetch();
            }
        });
    }

    handleDelta(selector, args)
    {
        const previousRevision = Number(args[1]);
        const revision = Number(args[2]);
        if (!isFinite(previousRevision) || !isFinite(revision))
        {
            return;
        }
        if (!this.snapshot || this.snapshot.revision !== previousRevision)
        {
            this.requiredRevision = Math.max(this.requiredRevision, revision);
            if (!this.fetchPending)
            {
                this.fetch();
            }
            return;
        }
        if (!this.deltaApplier.apply(this.snapshot, selector, args))
        {
            return;
        }

        this.snapshot.revision = revision;
        this.notify(this.snapshot, { selector: selector, args: args });
    }

    handleProcessorMarkersChanged(args)
    {
        if (!this.snapshot)
        {
            return;
        }

        const instanceIds = this.deltaApplier.applyProcessorMarkers(this.snapshot, args);
        if (instanceIds.length > 0)
        {
            this.notify(this.snapshot, {
                selector: "registry_processor_markers_changed",
                args: args,
                instanceIds: instanceIds
            });
        }
    }

    handleDone(args)
    {
        const requestId = String(args[2]);
        const response = this.assembler.take(requestId);
        if (!response)
        {
            return;
        }
        if (response.revision < this.requiredRevision ||
                this.snapshot && response.revision < this.snapshot.revision)
        {
            this.protocol.complete(requestId, { snapshot: this.snapshot });
            return;
        }
        if (this.snapshot && response.revision === this.snapshot.revision &&
                !processorMarkersChanged(this.snapshot, response.instances))
        {
            this.protocol.complete(requestId, { snapshot: this.snapshot });
            return;
        }

        this.snapshot = {
            revision: response.revision,
            instances: response.instances,
            groups: response.groups
        };
        this.requiredRevision = 0;
        this.notify(this.snapshot);
        this.protocol.complete(requestId, { snapshot: this.snapshot });
    }

    subscribe(callback, immediate)
    {
        this.subscribers.push(callback);
        if (immediate && this.snapshot)
        {
            callback(this.snapshot);
        }
        return () => {
            this.subscribers = this.subscribers.filter((listener) => {
                return listener !== callback;
            });
        };
    }

    notify(snapshot, delta)
    {
        this.subscribers.slice().forEach((listener) => {
            listener(snapshot, delta);
        });
    }

    destroy()
    {
        this.unsubscribers.forEach((unsubscribe) => unsubscribe());
        this.unsubscribers = [];
        this.assembler.clear();
        this.snapshot = null;
        this.subscribers = [];
        this.fetchPending = false;
        this.requiredRevision = 0;
        this.protocol = null;
    }
}

module.exports = {
    RegistryClient: RegistryClient
};
