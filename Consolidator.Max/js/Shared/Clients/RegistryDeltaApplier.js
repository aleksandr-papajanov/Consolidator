const { rebuildGroupsForInstance } = require("./RegistryGroups.js");

class RegistryDeltaApplier
{
    apply(snapshot, selector, args)
    {
        const handlers = {
            registry_instance_added: () => this.addInstance(snapshot, args),
            registry_instance_removed: () => this.removeInstance(snapshot, args),
            registry_label_changed: () => this.changeLabel(snapshot, args),
            registry_instance_mute_changed: () => this.changeInstanceBoolean(snapshot, args, "mute"),
            registry_instance_solo_changed: () => this.changeInstanceBoolean(snapshot, args, "solo"),
            registry_instance_bypass_changed: () => this.changeInstanceBoolean(snapshot, args, "bypass"),
            registry_processor_changed: () => this.changeProcessor(snapshot, args),
            registry_bank_group_changed: () => this.changeBankGroup(snapshot, args),
            registry_bank_effect_changed: () => this.changeBankBoolean(snapshot, args, "effectActive"),
            registry_bank_bypass_changed: () => this.changeBankBoolean(snapshot, args, "bypassed")
        };
        const handler = handlers[selector];
        if (!handler)
        {
            return false;
        }

        handler();
        return true;
    }

    addInstance(snapshot, args)
    {
        const instanceId = args[3];
        const instance = {
            instanceId: instanceId,
            label: String(args[4]),
            mute: Number(args[5]) !== 0,
            solo: Number(args[6]) !== 0,
            bypass: Number(args[7]) !== 0,
            processors: [],
            banks: []
        };
        const processorCount = Number(args[8]);
        for (let index = 0; index < processorCount; index += 1)
        {
            const position = 9 + index * 3;
            instance.processors.push({
                processorId: String(args[position]),
                effectActive: Number(args[position + 1]) !== 0,
                markerActive: false,
                bypassed: Number(args[position + 2]) !== 0
            });
        }

        const bankCount = Number(args[9 + processorCount * 3]);
        for (let index = 0; index < bankCount; index += 1)
        {
            const position = 10 + processorCount * 3 + index * 4;
            instance.banks.push({
                bankId: args[position],
                groupId: args[position + 1] === "none" ? null : args[position + 1],
                effectActive: Number(args[position + 2]) !== 0,
                bypassed: Number(args[position + 3]) !== 0
            });
        }
        snapshot.instances.push(instance);
        rebuildGroupsForInstance(snapshot, instanceId);
    }

    removeInstance(snapshot, args)
    {
        const instanceId = String(args[3]);
        snapshot.instances = snapshot.instances.filter((instance) => {
            return String(instance.instanceId) !== instanceId;
        });
        rebuildGroupsForInstance(snapshot, instanceId);
    }

    changeLabel(snapshot, args)
    {
        const instance = this.findInstance(snapshot, args[3]);
        if (instance)
        {
            instance.label = String(args[4]);
        }
    }

    changeInstanceBoolean(snapshot, args, property)
    {
        const instance = this.findInstance(snapshot, args[3]);
        if (instance)
        {
            instance[property] = Number(args[4]) !== 0;
        }
    }

    changeBankGroup(snapshot, args)
    {
        const instance = this.findInstance(snapshot, args[3]);
        const bank = this.findBank(instance, args[4]);
        if (bank)
        {
            bank.groupId = args[5] === "none" ? null : args[5];
            rebuildGroupsForInstance(snapshot, args[3]);
        }
    }

    changeBankBoolean(snapshot, args, property)
    {
        const bank = this.findBank(this.findInstance(snapshot, args[3]), args[4]);
        if (bank)
        {
            bank[property] = Number(args[5]) !== 0;
        }
    }

    changeProcessor(snapshot, args)
    {
        const instance = this.findInstance(snapshot, args[3]);
        const processor = instance && (instance.processors || []).find((candidate) => {
            return candidate.processorId === String(args[4]);
        });
        if (processor)
        {
            processor.effectActive = Number(args[5]) !== 0;
            processor.bypassed = Number(args[6]) !== 0;
        }
    }

    applyProcessorMarkers(snapshot, args)
    {
        const changes = this.decodeProcessorMarkers(args);
        if (!changes)
        {
            return [];
        }

        const instanceIds = [];
        changes.forEach((change) => {
            const instance = this.findInstance(snapshot, change.instanceId);
            if (!instance)
            {
                return;
            }
            change.processors.forEach((changed) => {
                const processor = (instance.processors || []).find((candidate) => {
                    return candidate.processorId === changed.processorId;
                });
                if (processor)
                {
                    processor.markerActive = changed.active;
                }
            });
            instanceIds.push(change.instanceId);
        });
        return instanceIds;
    }

    decodeProcessorMarkers(args)
    {
        if (!Array.isArray(args) || args.length < 2 || Number(args[0]) !== 1)
        {
            return null;
        }

        const instanceCount = Number(args[1]);
        if (!Number.isInteger(instanceCount) || instanceCount < 0)
        {
            return null;
        }

        const changes = [];
        let position = 2;
        for (let instanceIndex = 0; instanceIndex < instanceCount; instanceIndex += 1)
        {
            if (position + 1 >= args.length)
            {
                return null;
            }
            const instanceId = String(args[position]);
            const processorCount = Number(args[position + 1]);
            if (!Number.isInteger(processorCount) || processorCount < 0 ||
                    position + 2 + processorCount * 2 > args.length)
            {
                return null;
            }
            position += 2;
            const processors = [];
            for (let processorIndex = 0; processorIndex < processorCount; processorIndex += 1)
            {
                processors.push({
                    processorId: String(args[position]),
                    active: Number(args[position + 1]) !== 0
                });
                position += 2;
            }
            changes.push({ instanceId: instanceId, processors: processors });
        }

        return position === args.length ? changes : null;
    }

    findInstance(snapshot, instanceId)
    {
        return (snapshot.instances || []).find((candidate) => {
            return String(candidate.instanceId) === String(instanceId);
        });
    }

    findBank(instance, bankId)
    {
        return instance && (instance.banks || []).find((candidate) => {
            return Number(candidate.bankId) === Number(bankId);
        });
    }
}

module.exports = {
    RegistryDeltaApplier: RegistryDeltaApplier
};
