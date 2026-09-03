function isGroupedBank(bank)
{
    if (!bank || bank.groupId === undefined || bank.groupId === null) return false;
    let groupId = Number(bank.groupId);
    return isFinite(groupId) && groupId >= 0;
}

function groupLabel(groupId)
{
    let value = Number(groupId);
    if (!isFinite(value) || value < 0) return "";
    value = Math.floor(value);
    let label = "";
    do {
        label = String.fromCharCode(65 + value % 26) + label;
        value = Math.floor(value / 26) - 1;
    }
    while (value >= 0);
    return label;
}

function fillRectangle(graphics, color, x, y, width, height)
{
    graphics.set_source_rgba.apply(graphics, color);
    graphics.rectangle(x, y, width, height);
    graphics.fill();
}

module.exports = {
    fillRectangle: fillRectangle,
    groupLabel: groupLabel,
    isGroupedBank: isGroupedBank
};
