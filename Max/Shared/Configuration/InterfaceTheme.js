var InterfaceTheme = {
    colors: {
        background: [0.075, 0.075, 0.075, 1.0],
        transparent: [0.0, 0.0, 0.0, 0.0],
        surface: [0.11, 0.11, 0.11, 1.0],
        surfaceActive: [0.18, 0.18, 0.18, 1.0],
        surfaceInactive: [0.09, 0.09, 0.09, 1.0],
        track: [0.16, 0.16, 0.16, 1.0],
        trackLimited: [0.20, 0.20, 0.20, 1.0],
        border: [0.34, 0.34, 0.34, 1.0],
        borderActive: [0.10, 0.78, 0.92, 1.0],
        borderInactive: [0.22, 0.22, 0.22, 1.0],
        text: [0.82, 0.82, 0.82, 1.0],
        textMuted: [0.65, 0.7, 0.7, 1.0],
        textInactive: [0.42, 0.42, 0.42, 1.0],
        textDisabled: [0.28, 0.28, 0.28, 1.0],
        primaryAccent: [0.98, 0.72, 0.18, 1.0],
        secondaryAccent: [0.10, 0.78, 0.92, 1.0],
        secondaryAccentInactive: [0.20, 0.42, 0.46, 1.0],
        tertiaryAccent: [0.94, 0.30, 0.22, 1.0],
        tertiaryAccentVariant: [0.94, 0.30, 0.22, 0.25],
        selection: [0.96, 0.32, 0.78, 1.0],
        linkPalette: [
            [0.4353, 0.8471, 0.7373, 1.0],
            [0.4392, 0.7686, 0.7725, 1.0],
            [0.4392, 0.6902, 0.8039, 1.0],
            [0.4392, 0.6078, 0.8275, 1.0],
            [0.4353, 0.5216, 0.8510, 1.0],
            [0.4824, 0.4902, 0.8196, 1.0],
            [0.5725, 0.5216, 0.7412, 1.0],
            [0.6667, 0.5412, 0.6549, 1.0],
            [0.7569, 0.5569, 0.5608, 1.0],
            [0.8471, 0.5647, 0.4510, 1.0],
            [0.9020, 0.6500, 0.4000, 1.0],
            [0.8500, 0.7600, 0.3300, 1.0],
            [0.6600, 0.8100, 0.3600, 1.0],
            [0.3900, 0.8200, 0.5100, 1.0],
            [0.3000, 0.7700, 0.6900, 1.0],
            [0.8100, 0.4500, 0.7200, 1.0]
        ]
    },
    typography: {
        fontFamily: "Ableton Sans Medium",
        regularWeight: "normal",
        activeWeight: "bold",
        minimumSize: 10
    },
    geometry: {
        controlPaddingRatio: 0.0,
        controlGapRatio: 0.04,
        cornerRadiusRatio: 0.0,
        controlLineWidth: 2.0,
        indicatorLineWidth: 2.0,
        borderLineWidth: 1.0,
        markerRadiusRatio: 0.04,
        arrowSizeRatio: 0.11,
        valueGap: 3.0,
        minimumPadding: 0.0,
        maximumPadding: 0.0
    },
    controls: {
        button: {
            cornerRadiusRatio: 0.04,
            paddingRatio: 0.0
        },
        buttonGroup: {
            paddingRatio: 0.0,
            gapRatio: 0.0,
            contentPadding: 4.0,
            cornerRadiusRatio: 0.04
        },
        dial: {
            emptySectionAngle: 120 * Math.PI / 180,
            emptySectionCenterAngle: Math.PI / 2,
            arcBoundsHeightRatio: 1.72,
            ringGapRatio: 0.20,
            ringLineWidthDecay: 1.0,
            indicatorGap: 1.0,
            valuePaddingRatio: 0.56,
            activityButtonRadiusRatio: 0.08,
            activityButtonGap: 2.0,
            activityButtonLineWidth: 1.5
        },
        slider: {
            paddingRatio: 0.0,
            valueAreaRatio: 0.34
        }
    }
};

InterfaceTheme.states = {
    active: {
        fill: InterfaceTheme.colors.secondaryAccent,
        border: InterfaceTheme.colors.secondaryAccent,
        text: InterfaceTheme.colors.background
    },
    inactive: {
        fill: InterfaceTheme.colors.transparent,
        border: InterfaceTheme.colors.secondaryAccent,
        text: InterfaceTheme.colors.secondaryAccent
    },
    disabled: {
        fill: InterfaceTheme.colors.transparent,
        border: InterfaceTheme.colors.borderInactive,
        text: InterfaceTheme.colors.textDisabled
    },
    selected: {
        fill: InterfaceTheme.colors.secondaryAccent,
        border: InterfaceTheme.colors.secondaryAccent,
        text: InterfaceTheme.colors.background
    }
};
