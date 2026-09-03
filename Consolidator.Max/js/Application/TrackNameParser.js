function parseTrackName(args)
{
    let values = args || [];
    if (values.length && String(values[0]) === "name") {
        values = values.slice(1);
    }
    let name = values.join(" ");
    if (name.length >= 2 && name.charAt(0) === '"' &&
            name.charAt(name.length - 1) === '"') {
        return name.substring(1, name.length - 1);
    }
    return name;
}

module.exports = {
    parseTrackName: parseTrackName
};
