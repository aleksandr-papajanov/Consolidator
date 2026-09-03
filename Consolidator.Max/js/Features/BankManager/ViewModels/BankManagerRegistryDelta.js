const { bankGroupColor, selectionKey } = require("./BankManagerRows.js");

function applyBankManagerRegistryDelta(viewModel, snapshot, delta)
{
    if (delta.selector === "registry_processor_markers_changed")
    {
        return applyProcessorMarkers(viewModel, snapshot, delta);
    }

    const args = delta.args;
    const instanceId = String(args[3]);
    if (delta.selector === "registry_instance_added")
    {
        const added = findInstance(snapshot, instanceId);
        if (!added)
        {
            return false;
        }
        viewModel.rows.push(viewModel.createRow(added));
        delta.rowIndex = viewModel.rows.length - 1;
        return true;
    }

    const rowIndex = viewModel.findRowIndex(instanceId);
    if (delta.selector === "registry_instance_removed")
    {
        if (rowIndex >= 0)
        {
            delta.rowIndex = rowIndex;
            viewModel.rows.splice(rowIndex, 1);
        }
        return true;
    }
    if (rowIndex < 0)
    {
        return false;
    }

    delta.rowIndex = rowIndex;
    const row = viewModel.rows[rowIndex];
    if (delta.selector === "registry_label_changed")
    {
        row.label = String(args[4]);
    }
    else if (delta.selector === "registry_instance_mute_changed")
    {
        row.mute = Number(args[4]) !== 0;
    }
    else if (delta.selector === "registry_instance_solo_changed")
    {
        row.solo = Number(args[4]) !== 0;
    }
    else if (delta.selector === "registry_instance_bypass_changed")
    {
        row.bypass = Number(args[4]) !== 0;
    }
    else if (delta.selector === "registry_bank_group_changed")
    {
        applyBankGroup(viewModel, row, instanceId, args);
    }
    else if (delta.selector === "registry_bank_effect_changed")
    {
        applyBankBoolean(row, args, "effectActive");
    }
    else if (delta.selector === "registry_bank_bypass_changed")
    {
        applyBankBoolean(row, args, "bypassed");
    }
    else if (delta.selector === "registry_processor_changed")
    {
        applyProcessor(row, args);
    }
    else
    {
        return false;
    }
    return true;
}

function applyProcessorMarkers(viewModel, snapshot, delta)
{
    delta.rowIndices = [];
    (delta.instanceIds || []).forEach((instanceId) => {
        const rowIndex = viewModel.findRowIndex(instanceId);
        const instance = findInstance(snapshot, instanceId);
        if (rowIndex < 0 || !instance)
        {
            return;
        }
        const markers = {};
        (instance.processors || []).forEach((processor) => {
            markers[processor.processorId] = Boolean(processor.markerActive);
        });
        (viewModel.rows[rowIndex].processors || []).forEach((processor) => {
            processor.markerActive = Boolean(markers[processor.processorId]);
        });
        delta.rowIndices.push(rowIndex);
    });
    return true;
}

function applyBankGroup(viewModel, row, instanceId, args)
{
    const bankId = Number(args[4]);
    const bank = findBank(row, bankId);
    if (!bank)
    {
        return;
    }
    bank.groupId = args[5] === "none" ? null : args[5];
    bank.color = bank.groupId === null ? null : bankGroupColor(bank.groupId);
    if (bank.groupId !== null)
    {
        delete viewModel.selectedBanks[selectionKey(instanceId, bankId)];
    }
}

function applyBankBoolean(row, args, property)
{
    const bank = findBank(row, args[4]);
    if (bank)
    {
        bank[property] = Number(args[5]) !== 0;
    }
}

function applyProcessor(row, args)
{
    const processorId = String(args[4]);
    const processor = (row.processors || []).find((candidate) => {
        return candidate.processorId === processorId;
    });
    if (processor)
    {
        processor.effectActive = Number(args[5]) !== 0;
        processor.bypassed = Number(args[6]) !== 0;
    }
}

function findInstance(snapshot, instanceId)
{
    return (snapshot.instances || []).find((candidate) => {
        return String(candidate.instanceId) === String(instanceId);
    });
}

function findBank(row, bankId)
{
    return (row.banks || []).find((candidate) => {
        return Number(candidate.bankId) === Number(bankId);
    });
}

module.exports = {
    applyBankManagerRegistryDelta: applyBankManagerRegistryDelta
};
