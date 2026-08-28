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
        protocol.on("registry_bank_group_changed", this.handleDelta.bind(this, "registry_bank_group_changed"));
        protocol.on("registry_bank_effect_changed", this.handleDelta.bind(this, "registry_bank_effect_changed"));
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
        } else if (selector === "registry_bank_group_changed") {
            this.applyBankGroupChanged(args);
        } else if (selector === "registry_bank_effect_changed") {
            this.applyBankEffectChanged(args);
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
            banks: []
        };
        let count = Number(args[7]);
        for (let index = 0; index < count; index += 1) {
            let position = 8 + index * 3;
            instance.banks.push({
                bankId: args[position],
                groupId: args[position + 1] === "none" ? null : args[position + 1],
                effectActive: Number(args[position + 2]) !== 0
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
            banks: []
        };
        response.instances.push(instance);
        response.instancesById[String(instance.instanceId)] = instance;
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
            effectActive: Number(args[6]) !== 0
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
    
        if (this.snapshot && response.revision <= this.snapshot.revision) {
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
