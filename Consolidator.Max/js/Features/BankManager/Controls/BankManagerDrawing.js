const { UiColors } = require("../../../Shared/Theme/UiColors.js");
const { BankManagerControlOptions } = require("./BankManagerControlOptions.js");

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

function paintBypassIndicator(graphics, x, y)
{
    graphics.set_source_rgba.apply(graphics, UiColors.devices.mute);
    graphics.set_line_width(1);
    graphics.move_to(x + 3, y + 3);
    graphics.line_to(x + BankManagerControlOptions.bankSize - 3,
        y + BankManagerControlOptions.bankSize - 3);
    graphics.stroke();
    graphics.move_to(x + BankManagerControlOptions.bankSize - 3, y + 3);
    graphics.line_to(x + 3, y + BankManagerControlOptions.bankSize - 3);
    graphics.stroke();
}

module.exports = {
    fillRectangle: fillRectangle,
    groupLabel: groupLabel,
    isGroupedBank: isGroupedBank,
    paintBypassIndicator: paintBypassIndicator
};
