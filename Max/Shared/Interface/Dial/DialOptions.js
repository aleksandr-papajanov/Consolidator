include("../../Configuration/InterfaceTheme.js");

var DialOptions = {
    maximumValueCount: 3,
    defaultValue: 0.5,
    emptySectionAngle: InterfaceTheme.controls.dial.emptySectionAngle,
    emptySectionCenterAngle: InterfaceTheme.controls.dial.emptySectionCenterAngle,
    arcBoundsHeightRatio: InterfaceTheme.controls.dial.arcBoundsHeightRatio,
    ringGapRatio: InterfaceTheme.controls.dial.ringGapRatio,
    ringLineWidthDecay: InterfaceTheme.controls.dial.ringLineWidthDecay,
    indicatorGap: InterfaceTheme.controls.dial.indicatorGap,
    valuePaddingRatio: InterfaceTheme.controls.dial.valuePaddingRatio,
    activityButtonRadiusRatio: InterfaceTheme.controls.dial.activityButtonRadiusRatio,
    activityButtonGap: InterfaceTheme.controls.dial.activityButtonGap,
    activityButtonLineWidth: InterfaceTheme.controls.dial.activityButtonLineWidth,
    dragSensitivity: 0.007
};

DialOptions.startAngle = DialOptions.emptySectionCenterAngle
    + DialOptions.emptySectionAngle * 0.5;
DialOptions.endAngle = DialOptions.emptySectionCenterAngle
    - DialOptions.emptySectionAngle * 0.5
    + Math.PI * 2.0;
