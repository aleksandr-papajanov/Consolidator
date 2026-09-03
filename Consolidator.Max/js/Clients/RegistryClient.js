class RegistryClient
{
    constructor(protocol)
    {
        this.protocol = protocol;
        this.snapshot = null;
        this.subscribers = [];
        this.responses = {};
        this.fetchPending = false;
        this.requiredRevision = 0;
    
        protocol.on("registry_begin", this.handleBegin.bind(this));
        protocol.on("registry_instance", this.handleInstance.bind(this));
        protocol.on("registry_bank", this.handleBank.bind(this));
        protocol.on("registry_group", this.handleGroup.bind(this));
        protocol.on("registry_member", this.handleMember.bind(this));
        protocol.on("registry_done", this.handleDone.bind(this));
        protocol.on("registry_instance_added", this.handleDelta.bind(this, "registry_instance_added"));
        protocol.on("registry_instance_removed", this.handleDelta.bind(this, "registry_instance_removed"));
        protocol.on("registry_label_changed", this.handleDelta.bind(this, "registry_label_changed"));
        protocol.on("registry_instance_mute_changed", this.handleDelta.bind(this, "registry_instance_mute_changed"));
        protocol.on("registry_instance_solo_changed", this.handleDelta.bind(this, "registry_instance_solo_changed"));
        protocol.on("registry_instance_bypass_changed", this.handleDelta.bind(this, "registry_instance_bypass_changed"));
        protocol.on("registry_processor", this.handleProcessor.bind(this));
        protocol.on("registry_processor_changed", this.handleDelta.bind(this, "registry_processor_changed"));
        protocol.on("registry_processor_markers_changed", this.handleProcessorMarkersChanged.bind(this));
        protocol.on("registry_bank_group_changed", this.handleDelta.bind(this, "registry_bank_group_changed"));
        protocol.on("registry_bank_effect_changed", this.handleDelta.bind(this, "registry_bank_effect_changed"));
        protocol.on("registry_bank_bypass_changed", this.handleDelta.bind(this, "registry_bank_bypass_changed"));
        protocol.on("error", this.handleError.bind(this));
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
            if (callback) callback(response.error ? undefined : this.snapshot, response);
            if (!response.error &&
                (!this.snapshot ||
                 this.snapshot.revision < this.requiredRevision)) {
                this.fetch();
            }
        });
    }
    
    handleDelta(selector, args)
    {
        if (selector === "registry_label_changed" && typeof post === "function")
        {
            post("[Consolidator][TrackName] RegistryClient delta=" +
                JSON.stringify(args) + "\n");
        }
        let previousRevision = Number(args[1]);
        let revision = Number(args[2]);
        if (!isFinite(previousRevision) || !isFinite(revision)) {
            return;
        }
        if (!this.snapshot || this.snapshot.revision !== previousRevision) {
            this.requiredRevision = Math.max(this.requiredRevision, revision);
            if (!this.fetchPending) this.fetch();
            return;
        }
    
        if (selector === "registry_instance_added") {
            this.applyInstanceAdded(args);
        } else if (selector === "registry_instance_removed") {
            this.applyInstanceRemoved(args);
        } else if (selector === "registry_label_changed") {
            this.applyLabelChanged(args);
        } else if (selector === "registry_instance_mute_changed") {
            this.applyInstanceBooleanChanged(args, "mute");
        } else if (selector === "registry_instance_solo_changed") {
            this.applyInstanceBooleanChanged(args, "solo");
        } else if (selector === "registry_instance_bypass_changed") {
            this.applyInstanceBooleanChanged(args, "bypass");
        } else if (selector === "registry_processor_changed") {
            this.applyProcessorChanged(args);
        } else if (selector === "registry_bank_group_changed") {
            this.applyBankGroupChanged(args);
        } else if (selector === "registry_bank_effect_changed") {
            this.applyBankEffectChanged(args);
        } else if (selector === "registry_bank_bypass_changed") {
            this.applyBankBypassChanged(args);
        } else {
            return;
        }
        this.snapshot.revision = revision;
        this.notify(this.snapshot, { selector: selector, args: args });
    }
    
    applyInstanceAdded(args)
    {
        let instanceId = args[3];
        let instance = {
            instanceId: instanceId,
            label: String(args[4]),
            mute: Number(args[5]) !== 0,
            solo: Number(args[6]) !== 0,
            bypass: Number(args[7]) !== 0,
            processors: [],
            banks: []
        };
        let processorCount = Number(args[8]);
        for (let index = 0; index < processorCount; index += 1) {
            let position = 8 + index * 3;
            instance.processors.push({
                processorId: String(args[position]),
                effectActive: Number(args[position + 1]) !== 0,
                markerActive: false,
                bypassed: Number(args[position + 2]) !== 0
            });
        }
        let count = Number(args[9 + processorCount * 3]);
        for (let index = 0; index < count; index += 1) {
            let position = 10 + processorCount * 3 + index * 4;
            instance.banks.push({
                bankId: args[position],
                groupId: args[position + 1] === "none" ? null : args[position + 1],
                effectActive: Number(args[position + 2]) !== 0,
                bypassed: Number(args[position + 3]) !== 0
            });
        }
        this.snapshot.instances.push(instance);
        this.updateGroupsForInstance(instanceId, this.snapshot.instances);
    }
    
    applyInstanceRemoved(args)
    {
        let instanceId = String(args[3]);
        this.snapshot.instances = this.snapshot.instances.filter((instance) => {
            return String(instance.instanceId) !== instanceId;
        });
        this.updateGroupsForInstance(instanceId, this.snapshot.instances);
    }
    
    applyLabelChanged(args)
    {
        let instanceId = String(args[3]);
        if (typeof post === "function")
        {
            post("[Consolidator][TrackName] RegistryClient apply instanceId=" +
                JSON.stringify(instanceId) + " label=" +
                JSON.stringify(args[4]) + "\n");
        }
        this.snapshot.instances.forEach((instance) => {
            if (String(instance.instanceId) === instanceId) instance.label = String(args[4]);
        });
    }

    applyInstanceBooleanChanged(args, property)
    {
        let instanceId = String(args[3]);
        this.snapshot.instances.forEach((instance) => {
            if (String(instance.instanceId) === instanceId) {
                instance[property] = Number(args[4]) !== 0;
            }
        });
    }
    
    applyBankGroupChanged(args)
    {
        let instanceId = String(args[3]);
        let bankId = Number(args[4]);
        let groupId = args[5] === "none" ? null : args[5];
        this.snapshot.instances.forEach((instance) => {
            if (String(instance.instanceId) !== instanceId) return;
            instance.banks.forEach((bank) => {
                if (Number(bank.bankId) === bankId) bank.groupId = groupId;
            });
        });
        this.updateGroupsForInstance(instanceId, this.snapshot.instances);
    }

    applyBankEffectChanged(args)
    {
        let instanceId = String(args[3]);
        let bankId = Number(args[4]);
        let effectActive = Number(args[5]) !== 0;
        this.snapshot.instances.forEach((instance) => {
            if (String(instance.instanceId) !== instanceId) return;
            instance.banks.forEach((bank) => {
                if (Number(bank.bankId) === bankId) bank.effectActive = effectActive;
            });
        });
    }

    applyBankBypassChanged(args)
    {
        let instanceId = String(args[3]);
        let bankId = Number(args[4]);
        let bypassed = Number(args[5]) !== 0;
        this.snapshot.instances.forEach((instance) => {
            if (String(instance.instanceId) !== instanceId) return;
            instance.banks.forEach((bank) => {
                if (Number(bank.bankId) === bankId) bank.bypassed = bypassed;
            });
        });
    }

    applyProcessorChanged(args)
    {
        let instanceId = String(args[3]);
        let processorId = String(args[4]);
        this.snapshot.instances.forEach((instance) => {
            if (String(instance.instanceId) !== instanceId) return;
            instance.processors.forEach((processor) => {
                if (processor.processorId === processorId) {
                    processor.effectActive = Number(args[5]) !== 0;
                    processor.bypassed = Number(args[6]) !== 0;
                }
            });
        });
    }

    handleProcessorMarkersChanged(args)
    {
        if (!Array.isArray(args) || args.length < 2 ||
                Number(args[0]) !== 1 || !this.snapshot) return;
        let instanceCount = Number(args[1]);
        if (!Number.isInteger(instanceCount) || instanceCount < 0) return;
        let changes = [];
        let position = 2;
        for (let instanceIndex = 0;
            instanceIndex < instanceCount;
            instanceIndex += 1) {
            if (position + 1 >= args.length) return;
            let instanceId = String(args[position]);
            let processorCount = Number(args[position + 1]);
            if (!Number.isInteger(processorCount) || processorCount < 0 ||
                    position + 2 + processorCount * 2 > args.length) return;
            position += 2;
            let processors = [];
            for (let processorIndex = 0;
                processorIndex < processorCount;
                processorIndex += 1) {
                processors.push({
                    processorId: String(args[position]),
                    active: Number(args[position + 1]) !== 0
                });
                position += 2;
            }
            changes.push({ instanceId: instanceId, processors: processors });
        }
        if (position !== args.length) return;

        let instanceIds = [];
        changes.forEach((change) => {
            let instance = this.snapshot.instances.filter((candidate) => {
                return String(candidate.instanceId) === change.instanceId;
            })[0];
            if (!instance) return;
            change.processors.forEach((changedProcessor) => {
                instance.processors.forEach((processor) => {
                    if (processor.processorId === changedProcessor.processorId) {
                        processor.markerActive = changedProcessor.active;
                    }
                });
            });
            instanceIds.push(change.instanceId);
        });
        if (instanceIds.length > 0) {
            this.notify(this.snapshot, {
                selector: "registry_processor_markers_changed",
                args: args,
                instanceIds: instanceIds
            });
        }
    }
    
    updateGroupsForInstance(instanceId, instances)
    {
        let groups = this.snapshot.groups || [];
        groups.forEach((group) => {
            group.members = group.members.filter((member) => {
                return String(member.instanceId) !== String(instanceId);
            });
        });
        let instance = instances.filter((candidate) => {
            return String(candidate.instanceId) === String(instanceId);
        })[0];
        if (instance) {
            instance.banks.forEach((bank) => {
                if (bank.groupId === null || bank.groupId === undefined) return;
                let group = groups.filter((candidate) => {
                    return String(candidate.groupId) === String(bank.groupId);
                })[0];
                if (!group) {
                    group = { groupId: bank.groupId, members: [] };
                    groups.push(group);
                }
                group.members.push({ instanceId: instance.instanceId, bankId: bank.bankId });
            });
        }
        this.snapshot.groups = groups.filter((group) => {
            return group.members.length > 0;
        });
    }
    
    subscribe(callback, immediate)
    {
        this.subscribers.push(callback);
        if (immediate && this.snapshot) callback(this.snapshot);
        return () => {
            this.subscribers = this.subscribers.filter((listener) => {
                return listener !== callback;
            });
        };
    }
    
    handleBegin(args)
    {
        let requestId = String(args[2]);
        this.responses[requestId] = {
            revision: Number(args[3]),
            instances: [],
            groups: [],
            instancesById: {},
            groupsById: {}
        };
    }
    
    handleInstance(args)
    {
        let response = this.responses[String(args[2])];
        if (!response) return;
        let instance = {
            instanceId: args[3],
            label: String(args[4]),
            mute: Number(args[5]) !== 0,
            solo: Number(args[6]) !== 0,
            bypass: Number(args[7]) !== 0,
            processors: [],
            banks: []
        };
        response.instances.push(instance);
        response.instancesById[String(instance.instanceId)] = instance;
    }
    
    handleProcessor(args)
    {
        let response = this.responses[String(args[2])];
        if (!response) return;
        let instance = response.instancesById[String(args[3])];
        if (!instance) return;
        instance.processors.push({
            processorId: String(args[4]),
            effectActive: Number(args[5]) !== 0,
            markerActive: Number(args[6]) !== 0,
            bypassed: Number(args[7]) !== 0
        });
    }

    handleBank(args)
    {
        let response = this.responses[String(args[2])];
        if (!response) return;
        let instance = response.instancesById[String(args[3])];
        if (!instance) return;
        instance.banks.push({
            bankId: args[4],
            groupId: args[5] === "none" ? null : args[5],
            effectActive: Number(args[6]) !== 0,
            bypassed: Number(args[7]) !== 0
        });
    }
    
    handleGroup(args)
    {
        let response = this.responses[String(args[2])];
        if (!response) return;
        let group = { groupId: args[3], members: [] };
        response.groups.push(group);
        response.groupsById[String(group.groupId)] = group;
    }
    
    handleMember(args)
    {
        let response = this.responses[String(args[2])];
        if (!response) return;
        let group = response.groupsById[String(args[3])];
        if (!group) return;
        group.members.push({ instanceId: args[4], bankId: args[5] });
    }
    
    handleDone(args)
    {
        let requestId = String(args[2]);
        let response = this.responses[requestId];
        if (!response) return;
        delete this.responses[requestId];
    
        if (response.revision < this.requiredRevision) {
            this.protocol.complete(requestId, { snapshot: this.snapshot });
            return;
        }
    
        if (this.snapshot && response.revision < this.snapshot.revision) {
            this.protocol.complete(requestId, { snapshot: this.snapshot });
            return;
        }
        if (this.snapshot && response.revision === this.snapshot.revision &&
                !this.processorMarkersChanged(response.instances)) {
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

    processorMarkersChanged(instances)
    {
        let current = {};
        let next = {};
        (this.snapshot.instances || []).forEach((instance) => {
            (instance.processors || []).forEach((processor) => {
                current[String(instance.instanceId) + ":" + processor.processorId] =
                    Boolean(processor.markerActive);
            });
        });
        (instances || []).forEach((instance) => {
            (instance.processors || []).forEach((processor) => {
                next[String(instance.instanceId) + ":" + processor.processorId] =
                    Boolean(processor.markerActive);
            });
        });
        let currentKeys = Object.keys(current);
        let nextKeys = Object.keys(next);
        if (currentKeys.length !== nextKeys.length) return true;
        return currentKeys.some((key) => {
            return !Object.prototype.hasOwnProperty.call(next, key) ||
                current[key] !== next[key];
        });
    }
    
    handleError(args)
    {
        let requestId = String(args[2]);
        delete this.responses[requestId];
    }
    
    notify(snapshot, delta)
    {
        let listeners = this.subscribers.slice();
        for (let index = 0; index < listeners.length; index += 1) {
            listeners[index](snapshot, delta);
        }
    }
    
    destroy()
    {
        this.snapshot = null;
        this.subscribers = [];
        this.responses = {};
        this.fetchPending = false;
        this.requiredRevision = 0;
    }
}

module.exports = {
    RegistryClient: RegistryClient
};
