function AnalyzerViewConfig() {
    this.spectrum = {
        scaleDb: 24,
        scaleOptionsDb: [12, 24, 36],
        minimumFrequencyHz: 20,
        maximumFrequencyHz: 20000,
        background: { r: 0.07, g: 0.07, b: 0.07, a: 1 },
        grid: { r: 0.20, g: 0.20, b: 0.20, a: 1 },
        majorGrid: { r: 0.32, g: 0.32, b: 0.32, a: 1 },
        label: { r: 0.70, g: 0.70, b: 0.70, a: 1 },
        current: { r: 0.25, g: 0.85, b: 1.00, a: 1 },
        reference: { r: 1.00, g: 0.90, b: 0.30, a: 1 },
        fit: { r: 0.95, g: 0.45, b: 1.00, a: 1 },
        total: { r: 0.82, g: 0.82, b: 0.82, a: 1 },
        filter: { r: 0.86, g: 0.42, b: 0.72, a: 0.65 },
        handle: { r: 1.00, g: 1.00, b: 1.00, a: 1 },
        lineWidth: 1,
        totalLineWidth: 2,
        currentLineWidth: 1.5,
        filterLineWidth: 1.5,
        handleRadius: 5,
        handleHitRadius: 18,
        gainHandleX: 8,
        labelSize: 9,
        controlHeight: 18,
        controlPadding: 4,
        controlActive: { r: 0.96, g: 0.78, b: 0.18, a: 1 },
        controlInactive: { r: 0.96, g: 0.78, b: 0.18, a: 1 },
        controlLineWidth: 1,
        bottomPadding: 18,
        majorFrequencies: [100, 1000, 10000],
        minorFrequencies: [20, 50, 200, 500, 2000, 5000],
        dbStep: 12
    };
    this.analysis = {
        background: { r: 0.07, g: 0.07, b: 0.07, a: 1 },
        grid: { r: 0.20, g: 0.20, b: 0.20, a: 1 },
        label: { r: 0.70, g: 0.70, b: 0.70, a: 1 },
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
        overall: { r: 1.00, g: 1.00, b: 1.00, a: 1 },
        normalizationSensitivity: 1.35,
        labelSize: 8
    };
}

var analyzerViewConfig = new AnalyzerViewConfig();
