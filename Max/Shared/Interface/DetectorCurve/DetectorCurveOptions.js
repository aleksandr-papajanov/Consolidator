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
    gridColor: [0.28, 0.28, 0.28, 0.72],
    filterColors: [
        [0.10, 0.78, 0.92, 0.65],
        [0.98, 0.72, 0.18, 0.65]
    ]
};
