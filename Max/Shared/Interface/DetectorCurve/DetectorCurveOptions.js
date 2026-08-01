include("../../Configuration/InterfaceTheme.js");

var detectorCurveOptions = {
    minimumFrequencyHz: 20.0,
    maximumFrequencyHz: 20000.0,
    sampleRate: 48000.0,
    minimumDb: -24.0,
    maximumDb: 24.0,
    pointCount: 72,
    padding: InterfaceTheme.geometry.minimumPadding,
    labelHeight: 8.0,
    gridLineWidth: InterfaceTheme.geometry.borderLineWidth,
    filterLineWidth: InterfaceTheme.geometry.indicatorLineWidth,
    listenLineWidth: InterfaceTheme.geometry.controlLineWidth,
    totalLineWidth: InterfaceTheme.geometry.controlLineWidth,
    listenMarkerRadius: 10.0,
    markerHitRadius: 5.0,
    neutralMarkerOpacity: 0.45,
    inactiveMarkerOpacity: 0.70,
    linkedMarkerOpacity: 0.42,
    linkedMarkerRadius: 4.0,
    linkedMarkerLineWidth: InterfaceTheme.geometry.borderLineWidth,
    gridColor: InterfaceTheme.colors.track,
    filterColors: [
        InterfaceTheme.colors.primaryAccent,
        InterfaceTheme.colors.primaryAccent
    ]
};
