const { UiColors } = require("../../../Shared/Theme/UiColors.js");

const BankManagerControlOptions = {
    background: UiColors.base.background,
    text: UiColors.base.text,
    focused: UiColors.controls.active,
    remote: UiColors.base.text,
    solo: UiColors.devices.solo,
    mute: UiColors.devices.mute,
    disabled: UiColors.base.disabledText,
    separator: UiColors.base.lines,
    rowHeight: 16,
    bankSize: 16,
    bankGap: 0,
    columnGap: 5,
    deviceColumnGap: 5,
    actionColumnWidth: 64,
    actionButtonHeight: 16,
    historyGroupGap: 8,
    outerPadding: 4,
    actionGap: 3,
    actionFlashDurationMs: 180,
    deviceColors: UiColors.devices.processors,
    processorMarkerIds: [
        "input",
        "saturator",
        "compressor",
        "equalizer",
        "polish",
        "output"
    ],
    fontSize: 11
};

module.exports = {
    BankManagerControlOptions: BankManagerControlOptions
};
