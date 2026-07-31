var ProcessorTelemetryOptions = {
    levels: {
        minimumDb: -60.0,
        maximumDb: 0.0,
        defaultTargetDb: -18.0,
        averagingMilliseconds: 1500,
        minimumAveragingMilliseconds: 1000,
        peakReleaseDb: 0.75
    },
    normalized: {
        smoothing: 0.78,
        peakRelease: 0.025
    },
    saturation: {
        sensitivity: 0.9,
        smoothing: 0.85
    },
    onsetMatch: {
        compressorReductionPerThresholdDb: 0.75,
        minimumSaturatorRatio: 0.0001,
        initialSaturation: 0.05
    },
    targets: {
        compressorReduction: { minimum: 0.0, maximum: 24.0, defaultValue: 3.0, step: 3.0 },
        saturatorPercent: { minimum: 0.0, maximum: 100.0, defaultValue: 2.0, step: 1.0 }
    },
    capture: {
        maximumDurationMilliseconds: 2000
    },
    telemetry: {
        timeoutMilliseconds: 150,
        compressorReductionMaximumDb: 24.0,
        resetIntervalMilliseconds: 20,
        resetSmoothing: 0.58,
        resetMinimumScale: 0.01
    }
};
