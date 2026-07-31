include("../../../Configuration/InterfaceTheme.js");

var analysisOptions = {
    background: AnalyzerColor(InterfaceTheme.colors.background),
    grid: AnalyzerColor(InterfaceTheme.colors.track),
    label: AnalyzerColor(InterfaceTheme.colors.textMuted),
    metricColors: [
        { r: 0.30, g: 0.78, b: 1.00, a: 0.9 },
        { r: 1.00, g: 0.55, b: 0.30, a: 0.9 },
        { r: 0.72, g: 0.50, b: 1.00, a: 0.9 },
        { r: 0.38, g: 0.90, b: 0.62, a: 0.9 },
        { r: 0.95, g: 0.80, b: 0.30, a: 0.9 },
        { r: 0.98, g: 0.36, b: 0.55, a: 0.9 },
        { r: 0.35, g: 0.85, b: 0.88, a: 0.9 },
        { r: 0.75, g: 0.75, b: 0.75, a: 0.9 }
    ],
    normalizationSensitivity: 1.35,
    plotTop: analyzerControlsOptions.controlHeight + analyzerControlsOptions.controlPadding,
    plotBottomPadding: 14,
    labelSize: InterfaceTheme.typography.minimumSize,
    fontFamily: InterfaceTheme.typography.fontFamily
};
