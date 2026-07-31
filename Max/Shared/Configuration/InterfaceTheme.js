var InterfaceTheme = {
    colors: {
        background: [0.075, 0.075, 0.075, 1.0],
        surface: [0.11, 0.11, 0.11, 1.0],
        surfaceActive: [0.18, 0.18, 0.18, 1.0],
        surfaceInactive: [0.09, 0.09, 0.09, 1.0],
        track: [0.20, 0.20, 0.20, 1.0],
        border: [0.34, 0.34, 0.34, 1.0],
        borderActive: [0.10, 0.78, 0.92, 1.0],
        borderInactive: [0.22, 0.22, 0.22, 1.0],
        text: [0.82, 0.82, 0.82, 1.0],
        textMuted: [0.65, 0.7, 0.7, 1.0],
        textInactive: [0.42, 0.42, 0.42, 1.0],
        textDisabled: [0.28, 0.28, 0.28, 1.0],
        accent: [0.10, 0.78, 0.92, 1.0],
        accentActive: [0.98, 0.72, 0.18, 1.0],
        accentInactive: [0.20, 0.42, 0.46, 1.0],
        value: [0.10, 0.78, 0.92, 1.0],
        indicator: [0.98, 0.72, 0.18, 1.0],
        levelPeak: [1.0, 0.52, 0.04, 0.50],
        levelSmooth: [0.02, 0.86, 1.0, 0.50],
        reductionPeak: [1.0, 0.52, 0.04, 0.50],
        reductionSmooth: [1.0, 0.04, 0.04, 0.50],
        reduction: [0.94, 0.30, 0.22, 1.0],
        alert: [0.92, 0.18, 0.14, 1.0],
        positive: [0.22, 0.94, 0.74, 1.0],
        selection: [0.96, 0.32, 0.78, 1.0],
        linkPalette: [
            [0.24, 0.76, 0.94, 1.0],
            [0.96, 0.38, 0.54, 1.0],
            [0.47, 0.84, 0.34, 1.0],
            [0.72, 0.48, 0.94, 1.0],
            [0.98, 0.52, 0.20, 1.0],
            [0.20, 0.86, 0.72, 1.0],
            [0.96, 0.78, 0.24, 1.0],
            [0.38, 0.58, 0.96, 1.0],
            [0.92, 0.34, 0.78, 1.0],
            [0.62, 0.86, 0.30, 1.0]
        ]
    },
    states: {
        active: {
            fill: [0.10, 0.78, 0.92, 1.0],
            border: [0.10, 0.78, 0.92, 1.0],
            text: [0.075, 0.075, 0.075, 1.0]
        },
        inactive: {
            fill: [0.0, 0.0, 0.0, 0.0],
            border: [0.10, 0.78, 0.92, 1.0],
            text: [0.10, 0.78, 0.92, 1.0]
        },
        disabled: {
            fill: [0.0, 0.0, 0.0, 0.0],
            border: [0.22, 0.22, 0.22, 1.0],
            text: [0.28, 0.28, 0.28, 1.0]
        },
        selected: {
            fill: [0.98, 0.72, 0.18, 1.0],
            border: [0.98, 0.72, 0.18, 1.0],
            text: [0.075, 0.075, 0.075, 1.0]
        }
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
        indicatorLineWidth: 1.5,
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
            gapRatio: 0.03,
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
