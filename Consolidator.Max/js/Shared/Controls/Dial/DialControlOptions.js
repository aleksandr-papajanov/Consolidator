const { UiColors } = require("../../Theme/UiColors.js");

const DialControlOptions = {
    startAngle: Math.PI * (5 / 6),
    endAngle: Math.PI * (13 / 6),
    lineWidth: 3,
    indicatorWidth: 2,
    dragSensitivity: 0.007,
    labelRestoreDelayMs: 500,
    background: UiColors.base.background,
    ring: UiColors.base.lines,
    active: UiColors.base.activeText,
    inactive: UiColors.base.lines,
    visualization: UiColors.controls.visualization
};

module.exports = {
    DialControlOptions: DialControlOptions
};
