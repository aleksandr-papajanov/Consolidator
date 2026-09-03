const { UiColors } = require("../../Theme/UiColors.js");

const MultiValueToggleOptions = {
    startAngle: Math.PI * (5 / 6),
    angleRange: Math.PI * (4 / 3),
    pointAngle: Math.PI / 3,
    indicatorWidth: 2,
    pointerStart: 0.2,
    pointerEnd: 0.6,
    dragSensitivity: 0.015,
    background: UiColors.base.background,
    ring: UiColors.base.lines,
    active: UiColors.base.activeText,
    inactive: UiColors.base.lines
};

function togglePoint(index, count, centerX, centerY, radius)
{
    let options = MultiValueToggleOptions;
    let span = Math.min(options.angleRange,
        options.pointAngle * Math.max(0, count - 1));
    let start = options.startAngle + (options.angleRange - span) * 0.5;
    let angle = start + (count <= 1
        ? span * 0.5 : span * index / (count - 1));
    return {
        x: centerX + Math.cos(angle) * radius,
        y: centerY + Math.sin(angle) * radius
    };
}

module.exports = {
    MultiValueToggleOptions: MultiValueToggleOptions,
    togglePoint: togglePoint
};
