function encodeStatePath(path)
{
    const parts = Array.isArray(path) ? path : path.split(".");
    return parts.map((part) => {
        return /^\d+$/.test(String(part)) ? parseInt(part, 10) : part;
    });
}

function encodeStateValue(path, value)
{
    const text = Array.isArray(path) ? path.join(".") : path;
    if (/(^|\.)bank\.[0-6]\.group$/.test(text) && value === null)
    {
        return "none";
    }
    return value;
}

function decodeStateValue(value)
{
    return value === "none" ? null : value;
}

function decodeOptionalStateValue(value)
{
    return value === "none" ? undefined : value;
}

module.exports = {
    decodeOptionalStateValue: decodeOptionalStateValue,
    decodeStateValue: decodeStateValue,
    encodeStatePath: encodeStatePath,
    encodeStateValue: encodeStateValue
};
