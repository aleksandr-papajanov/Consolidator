include("../../../Configuration/InterfaceTheme.js");

var analysisMetricColors = [];
for (var analysisColorIndex = 0;
    analysisColorIndex < InterfaceTheme.colors.linkPalette.length;
    ++analysisColorIndex) {
    analysisMetricColors.push(AnalyzerColor(
        InterfaceTheme.colors.linkPalette[analysisColorIndex]
    ));
}

var analysisOptions = {
    background: AnalyzerColor(InterfaceTheme.colors.background),
    grid: AnalyzerColor(InterfaceTheme.colors.track),
    label: AnalyzerColor(InterfaceTheme.colors.textMuted),
    metricColors: analysisMetricColors,
    normalizationSensitivity: 1.35,
    plotTop: analyzerControlsOptions.controlHeight + analyzerControlsOptions.controlPadding,
    plotBottomPadding: 14 + analyzerControlsOptions.controlHeight,
    labelSize: InterfaceTheme.typography.minimumSize,
    fontFamily: InterfaceTheme.typography.fontFamily
};
