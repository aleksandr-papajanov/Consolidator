class RegistrySnapshotAssembler
{
    constructor()
    {
        this.responses = {};
    }

    begin(args)
    {
        const requestId = String(args[2]);
        this.responses[requestId] = {
            revision: Number(args[3]),
            instances: [],
            groups: [],
            instancesById: {},
            groupsById: {}
        };
    }

    addInstance(args)
    {
        const response = this.response(args);
        if (!response)
        {
            return;
        }

        const instance = {
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

    addProcessor(args)
    {
        const response = this.response(args);
        const instance = response && response.instancesById[String(args[3])];
        if (!instance)
        {
            return;
        }

        instance.processors.push({
            processorId: String(args[4]),
            effectActive: Number(args[5]) !== 0,
            markerActive: Number(args[6]) !== 0,
            bypassed: Number(args[7]) !== 0
        });
    }

    addBank(args)
    {
        const response = this.response(args);
        const instance = response && response.instancesById[String(args[3])];
        if (!instance)
        {
            return;
        }

        instance.banks.push({
            bankId: args[4],
            groupId: args[5] === "none" ? null : args[5],
            effectActive: Number(args[6]) !== 0,
            bypassed: Number(args[7]) !== 0
        });
    }

    addGroup(args)
    {
        const response = this.response(args);
        if (!response)
        {
            return;
        }

        const group = { groupId: args[3], members: [] };
        response.groups.push(group);
        response.groupsById[String(group.groupId)] = group;
    }

    addMember(args)
    {
        const response = this.response(args);
        const group = response && response.groupsById[String(args[3])];
        if (group)
        {
            group.members.push({ instanceId: args[4], bankId: args[5] });
        }
    }

    take(requestId)
    {
        const key = String(requestId);
        const response = this.responses[key];
        delete this.responses[key];
        return response || null;
    }

    discard(requestId)
    {
        delete this.responses[String(requestId)];
    }

    response(args)
    {
        return this.responses[String(args[2])] || null;
    }

    clear()
    {
        this.responses = {};
    }
}

function processorMarkersChanged(currentSnapshot, nextInstances)
{
    const current = processorMarkers(currentSnapshot && currentSnapshot.instances);
    const next = processorMarkers(nextInstances);
    const currentKeys = Object.keys(current);
    const nextKeys = Object.keys(next);
    if (currentKeys.length !== nextKeys.length)
    {
        return true;
    }

    return currentKeys.some((key) => {
        return !Object.prototype.hasOwnProperty.call(next, key) ||
            current[key] !== next[key];
    });
}

function processorMarkers(instances)
{
    const markers = {};
    (instances || []).forEach((instance) => {
        (instance.processors || []).forEach((processor) => {
            markers[String(instance.instanceId) + ":" + processor.processorId] =
                Boolean(processor.markerActive);
        });
    });
    return markers;
}

module.exports = {
    RegistrySnapshotAssembler: RegistrySnapshotAssembler,
    processorMarkersChanged: processorMarkersChanged
};
