const { UiColors } = require("../../../Shared/Theme/UiColors.js");

function bankGroupColor(groupId)
{
    const colors = UiColors.groups.banks;
    let index = Number(groupId) % colors.length;
    if (index < 0)
    {
        index += colors.length;
    }
    return colors[index];
}

function bankHasGroup(bank)
{
    if (!bank || bank.groupId === undefined || bank.groupId === null)
    {
        return false;
    }
    const groupId = Number(bank.groupId);
    return isFinite(groupId) && groupId >= 0;
}

function createBankManagerRow(instance, localInstanceId, selectedBanks, focusedSelection)
{
    return {
        instanceId: instance.instanceId,
        label: instance.label,
        local: String(instance.instanceId) === String(localInstanceId),
        mute: Boolean(instance.mute),
        solo: Boolean(instance.solo),
        bypass: Boolean(instance.bypass),
        processors: (instance.processors || []).map((processor) => ({
            processorId: processor.processorId,
            effectActive: Boolean(processor.effectActive),
            markerActive: Boolean(processor.markerActive),
            bypassed: Boolean(processor.bypassed)
        })),
        banks: (instance.banks || []).map((bank) => {
            const bankId = Number(bank.bankId);
            const focused = Boolean(focusedSelection &&
                String(focusedSelection.instanceId) === String(instance.instanceId) &&
                bankId === Number(focusedSelection.bankId));
            return {
                bankId: bankId,
                label: String(bankId),
                system: false,
                visible: true,
                enabled: true,
                active: focused,
                focused: focused,
                selected: !bankHasGroup(bank) &&
                    Boolean(selectedBanks[selectionKey(instance.instanceId, bankId)]),
                groupId: bank.groupId,
                effectActive: Boolean(bank.effectActive),
                bypassed: Boolean(bank.bypassed),
                color: bankHasGroup(bank) ? bankGroupColor(bank.groupId) : null,
                opacity: 1
            };
        })
    };
}

function retainSelectableBanks(snapshot, selectedBanks)
{
    const retained = {};
    (snapshot.instances || []).forEach((instance) => {
        (instance.banks || []).forEach((bank) => {
            const key = selectionKey(instance.instanceId, bank.bankId);
            if (!bankHasGroup(bank) && selectedBanks[key])
            {
                retained[key] = {
                    instanceId: instance.instanceId,
                    bankId: Number(bank.bankId)
                };
            }
        });
    });
    return retained;
}

function selectionKey(instanceId, bankId)
{
    return String(instanceId) + ":" + String(bankId);
}

module.exports = {
    bankGroupColor: bankGroupColor,
    bankHasGroup: bankHasGroup,
    createBankManagerRow: createBankManagerRow,
    retainSelectableBanks: retainSelectableBanks,
    selectionKey: selectionKey
};
