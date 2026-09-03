function rebuildGroupsForInstance(snapshot, instanceId)
{
    let groups = snapshot.groups || [];
    groups.forEach((group) => {
        group.members = group.members.filter((member) => {
            return String(member.instanceId) !== String(instanceId);
        });
    });

    const instance = snapshot.instances.find((candidate) => {
        return String(candidate.instanceId) === String(instanceId);
    });
    if (instance)
    {
        (instance.banks || []).forEach((bank) => {
            if (bank.groupId === null || bank.groupId === undefined)
            {
                return;
            }

            let group = groups.find((candidate) => {
                return String(candidate.groupId) === String(bank.groupId);
            });
            if (!group)
            {
                group = { groupId: bank.groupId, members: [] };
                groups.push(group);
            }
            group.members.push({
                instanceId: instance.instanceId,
                bankId: bank.bankId
            });
        });
    }

    snapshot.groups = groups.filter((group) => group.members.length > 0);
}

module.exports = {
    rebuildGroupsForInstance: rebuildGroupsForInstance
};
