function colorFromArguments(hasColor, red, green, blue, alpha)
{
    return Number(hasColor) === 0
        ? null : [Number(red), Number(green), Number(blue), Number(alpha)];
}

function createRow(instanceId, label, local, solo, mute, bypass)
{
    return {
        instanceId: instanceId,
        label: String(label),
        local: Number(local) !== 0,
        solo: Number(solo) !== 0,
        mute: Number(mute) !== 0,
        bypass: Number(bypass) !== 0,
        processors: [],
        banks: []
    };
}

function createProcessor(processorId, effectActive, markerActive, bypassed)
{
    return {
        processorId: String(processorId),
        effectActive: Number(effectActive) !== 0,
        markerActive: Number(markerActive) !== 0,
        bypassed: Number(bypassed) !== 0
    };
}

function createBank(args, offset)
{
    offset = offset || 0;
    return {
        bankId: args[offset],
        label: String(args[offset + 1]),
        system: Number(args[offset + 2]) !== 0,
        visible: Number(args[offset + 3]) !== 0,
        enabled: Number(args[offset + 4]) !== 0,
        active: Number(args[offset + 5]) !== 0,
        selected: Number(args[offset + 6]) !== 0,
        opacity: Number(args[offset + 7]),
        groupId: Number(args[offset + 8]),
        effectActive: Number(args[offset + 9]) !== 0,
        bypassed: Number(args[offset + 10]) !== 0,
        color: colorFromArguments.apply(null, args.slice(offset + 11, offset + 16)),
        textColor: colorFromArguments.apply(
            null, args.slice(offset + 16, offset + 21))
    };
}

function createAction(enabled, active, color)
{
    return {
        enabled: Number(enabled) !== 0,
        active: Number(active) !== 0,
        color: color
    };
}

function createHistory(cursor, entryCount, canUndo, canRedo)
{
    return {
        cursor: Number(cursor),
        entryCount: Number(entryCount),
        canUndo: Number(canUndo) !== 0,
        canRedo: Number(canRedo) !== 0
    };
}

module.exports = {
    colorFromArguments: colorFromArguments,
    createAction: createAction,
    createBank: createBank,
    createHistory: createHistory,
    createProcessor: createProcessor,
    createRow: createRow
};
