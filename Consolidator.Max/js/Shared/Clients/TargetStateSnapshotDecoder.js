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

function decodeBankStateSnapshots(args)
{
    const bankCount = Number(args[5]);
    let offset = 6;
    const snapshots = [];
    let invalid = !Number.isInteger(bankCount) || bankCount < 0;
    for (let bankIndex = 0; !invalid && bankIndex < bankCount; bankIndex += 1)
    {
        if (offset + 2 > args.length)
        {
            invalid = true;
            break;
        }
        const bankId = Number(args[offset++]);
        const entryCount = Number(args[offset++]);
        if (!Number.isInteger(bankId) || bankId < 0 ||
                !Number.isInteger(entryCount) || entryCount < 0 ||
                offset + entryCount * 2 > args.length)
        {
            invalid = true;
            break;
        }
        const entries = [];
        for (let entryIndex = 0; entryIndex < entryCount; entryIndex += 1)
        {
            entries.push({
                path: String(args[offset++]),
                value: args[offset++]
            });
        }
        snapshots.push({ bankId: bankId, entries: entries });
    }
    return {
        instanceId: String(args[3]),
        snapshotContext: String(args[4]),
        snapshots: snapshots,
        invalid: invalid || offset !== args.length
    };
}

function optionalValue(value)
{
    return value === "none" ? undefined : value;
}

module.exports = {
    decodeTargetStateEntry: decodeTargetStateEntry,
    decodeTargetStateSnapshot: decodeTargetStateSnapshot,
    decodeBankStateSnapshots: decodeBankStateSnapshots
};
