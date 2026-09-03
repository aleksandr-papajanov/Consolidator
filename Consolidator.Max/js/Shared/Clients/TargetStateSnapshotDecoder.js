const TARGET_ENTRY_SIZE = 6;

function decodeTargetStateEntry(path, values, instanceId)
{
    return {
        path: String(path),
        value: values[0],
        status: values[1],
        physicalMin: optionalValue(values[2]),
        physicalMax: optionalValue(values[3]),
        min: optionalValue(values[4]),
        max: optionalValue(values[5]),
        instanceId: instanceId
    };
}

function decodeTargetStateSnapshot(args)
{
    const entryCount = Number(args[6]);
    const snapshot = {
        instanceId: String(args[3]),
        bankId: Number(args[4]),
        snapshotContext: String(args[5]),
        expected: entryCount,
        entries: [],
        invalid: !Number.isInteger(entryCount) || entryCount < 0 ||
            args.length !== 7 + entryCount * TARGET_ENTRY_SIZE
    };
    for (let index = 0; !snapshot.invalid && index < entryCount; index += 1)
    {
        const offset = 7 + index * TARGET_ENTRY_SIZE;
        snapshot.entries.push(decodeTargetStateEntry(
            args[offset],
            [args[offset + 1], "ready"].concat(
                args.slice(offset + 2, offset + TARGET_ENTRY_SIZE)
            ),
            snapshot.instanceId
        ));
    }
    return snapshot;
}

function optionalValue(value)
{
    return value === "none" ? undefined : value;
}

module.exports = {
    decodeTargetStateEntry: decodeTargetStateEntry,
    decodeTargetStateSnapshot: decodeTargetStateSnapshot
};
